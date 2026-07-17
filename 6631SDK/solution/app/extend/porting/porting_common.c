/*****************************************************************************
* 						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2015, All right reserved
******************************************************************************

******************************************************************************
* File Name :	porting_common.c
* Author    :	B.Z.
* Project   :	CM Module
* Type      :   Source Code
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
 VERSION	  Date			  AUTHOR         Description
  1.0	   2016.03.04		   B.Z.			  Creation
*****************************************************************************/
#include "gxcore.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdarg.h>
#include "module/app_log.h"

int common_thread_create(const char *ThreadName,
                         int *ThreadID,
                         void (*EntryFunc)(void*),
                         void *Arg,
                         unsigned int StackSize,
                         unsigned int Priority)
{
	int ret = 0;
	ret = GxCore_ThreadCreate(ThreadName,ThreadID,EntryFunc,Arg,StackSize,Priority);
    return ret;
}

int common_thread_delay(unsigned int Millisecond)
{
    int ret = 0;
	ret = GxCore_ThreadDelay(Millisecond);
    return ret;
}

int common_thread_join(int ThreadID)
{
    int ret = 0;
	ret = GxCore_ThreadJoin(ThreadID);
    return ret;
}

int common_thread_detach(void)
{
    int ret = 0;
    ret = GxCore_ThreadDetach();
    return ret;
}

int common_queue_create(int *QueueID, unsigned int QueueDepth, unsigned int DataSize)
{
	int ret = 0;
    ret = GxCore_QueueCreate(QueueID,QueueDepth,DataSize);
    return ret;
}

int common_queue_delete(int QueueID)
{
    int ret = 0;
	ret = GxCore_QueueDelete(QueueID);
    return ret;
}

int common_queue_receive(int QueueID,
                           void *DataBuf,
                           unsigned int Size,
                           unsigned int *SizeCopied,
                           int Timeout)
{// Timeout==0: means no timeout; Timeout==-1: special mode...
    int ret = -1;
    ret = GxCore_QueueGet(QueueID,DataBuf,Size,SizeCopied,Timeout);
    return ret;
}

int common_queue_send(int QueueID, void *DataBuf, unsigned int Size, int Timeout)
{// Timeout == 0: means no timeout; Timeout < 0: special mode...
    int ret = 0;
	ret = GxCore_QueuePut(QueueID,DataBuf,Size,Timeout);
    return ret;
}

int common_semaphore_create(int *SemId, unsigned int InitialValue)
{
    int ret = 0;
	ret = GxCore_SemCreate(SemId,InitialValue);
    return ret;
}

int common_semaphore_post(int SemID)
{
    int ret = 0;
	ret = GxCore_SemPost(SemID);
    return ret;
}

int common_semaphore_wait(int SemID)
{
    int ret = 0;
	ret = GxCore_SemWait(SemID);
    return ret;
}

int common_semaphore_delete(int SemID)
{
    int ret = 0;
	ret = GxCore_SemDelete(SemID);
    return ret;
}

int common_mutex_create(int *MutexID)
{
    int ret = 0;
	ret = GxCore_MutexCreate(MutexID);
    return ret;
}

int common_mutex_delete(int MutexID)
{
    int ret = 0;
	ret = GxCore_MutexDelete(MutexID);
    return ret;
}

int common_mutex_lock(int MutexID)
{
    int ret = 0;
	ret = GxCore_MutexLock(MutexID);
    return ret;
}

int common_mutex_unlock(int MutexID)
{
    int ret = 0;
	ret = GxCore_MutexUnlock(MutexID);
    return ret;
}

void *common_malloc(unsigned int Size)
{
    return GxCore_Mallocz(Size);
}

void common_free(void* ptr)
{
    GxCore_Free(ptr);
}

static unsigned char _ascii2hex(unsigned char ascii)
{
    if (ascii >= '0' && ascii <= '9')
    {
        return ascii - 0x30;
    }
    else if (ascii >= 'a' && ascii <= 'f')
    {
        return ascii - 0x61 + 0xa;
    }
    else if (ascii >= 'A' && ascii <= 'F')
    {
        return ascii - 0x41 + 0xa;
    }
    return 0;
}

unsigned int common_str2hex(unsigned int strlen, unsigned char *strbuf, unsigned char *numbuf)
{
    unsigned int hexlen = 0;
    unsigned int i;
    unsigned char num;

    for (i = 0; i < strlen / 2; i++)
    {
        num = _ascii2hex(strbuf[2 * i]) & 0x0f;
        num <<= 4;
        num |= _ascii2hex(strbuf[2 * i + 1]) & 0x0f;
        numbuf[hexlen++] = num;
    }
    return hexlen;
}


int common_show_time(char *str)
{
	SHOWTIME(str);
    return 0;
}

void common_printf(char *format, ...)
{
#define BUFFER_LEN  256
	char Buf[BUFFER_LEN];
	va_list args;
    memset(Buf,0, BUFFER_LEN);

	va_start(args, format);
	vsnprintf(Buf, BUFFER_LEN, format, args);
	va_end(args);

	app_log_info("%s", Buf);
}

//#define _UART_DEBUG

#ifdef _UART_DEBUG
#define UART_DBG app_log_debug
#else
#define UART_DBG(fmt, args...)   {;}
#endif
static int sg_UartFd = -1;

#ifdef ECOS_OS
void common_uart_printf(const char *fmt, ...)
{
#ifdef _UART_DEBUG
    va_list va;
    char Buff[512];

    if (sg_UartFd == -1)
    {
        UART_DBG("%s", fmt);
        return;
    }
    memset(Buff, '\0', 512);
    va_start(va, fmt);
    vsprintf(Buff, fmt, va);
    va_end(va);
    //write(sg_UartFd, Buff, 512);
    write(sg_UartFd, Buff, strlen(Buff));

    GxCore_ThreadDelay(10);
#endif
}

int common_uart_init(unsigned int Baudrate)
{
    int Ret = 0;
    unsigned int BRate = Baudrate;
    if (sg_UartFd > 0)
    {
        UART_DBG("\ncommon uart, uart init parameter error\n");
        return -1;
    }
    sg_UartFd = open("/dev/ser0", O_RDWR | O_NONBLOCK);
    if (sg_UartFd < 0)
    {
        UART_DBG("\ncommon uart, uart open failed\n");
        return -1;
    }
    Ret = ioctl(sg_UartFd, 0x0191/*CYG_IO_SET_CONFIG_SERIAL_BAUD_RATE*/, (void *)&BRate);
    if (Ret < 0)
    {
        common_uart_printf("\ncommon uart, uart set baudrate failed\n");
        return -1;
    }
    GxCore_PrintSwitch(false);
    return sg_UartFd;
}

int common_uart_receive(unsigned char *Buffer, unsigned int byteToRead)
{
    int RealByteRead = 0;
    if ((Buffer == NULL) || (sg_UartFd < 0))
    {
        common_uart_printf("\ncommon uart, uart init failed\n");
        return -1;
    }
    RealByteRead = read(sg_UartFd, Buffer, byteToRead);
    if (RealByteRead != byteToRead)
    {
        //common_uart_printf("\ncommon uart, uart read not enough data\n");
    }
    return RealByteRead;
}

int common_uart_send(unsigned char *Buffer, unsigned int byteToWrite)
{
    int RealByteGet = 0;
    if ((Buffer == NULL) || (sg_UartFd <= 0))
    {
        return -1;
    }
    RealByteGet = write(sg_UartFd, Buffer, byteToWrite);
    if (RealByteGet != byteToWrite)
    {
        common_uart_printf("\ncommon uart, uart send failed\n");
        return -1;
    }
    return RealByteGet;
}

int common_uart_close(void)
{
    if (sg_UartFd <= 0)
    {
        return -1;
    }
    close(sg_UartFd);
    sg_UartFd = -1;
    GxCore_PrintSwitch(true);
    return 0;
}

#endif

#ifdef LINUX_OS
#include "termios.h"

static int set_termios(int fd, long speed, char databit, char stopbit, char oebit, struct termios *pstTtyattr)
{
    int err = -1;

    pstTtyattr->c_cflag |= CLOCAL | CREAD;
    switch (databit)
    {
        case 7:
            pstTtyattr->c_cflag |= CS7;
            break;
        case 8:
            pstTtyattr->c_cflag |= CS8;
            break;
        default:
            pstTtyattr->c_cflag |= CS8;
            break;
    }
    switch (stopbit)
    {
        case 1:
            pstTtyattr->c_cflag &= ~CSTOPB;
            break;
        case 2:
            pstTtyattr->c_cflag |= CSTOPB;
            break;
        default:
            pstTtyattr->c_cflag &= ~CSTOPB;
            break;
    }
    switch (oebit)
    {
        case 'O'://odd
            pstTtyattr->c_cflag |= PARENB;
            pstTtyattr->c_cflag |= (INPCK | ISTRIP);
            pstTtyattr->c_cflag |= PARODD;
            break;
        case 'E'://even
            pstTtyattr->c_cflag |= PARENB;
            pstTtyattr->c_cflag |= (INPCK | ISTRIP);
            pstTtyattr->c_cflag &= ~PARODD;
            break;
        case 'N':
            pstTtyattr->c_cflag &= ~PARODD;
            break;
        default:
            pstTtyattr->c_cflag &= ~PARODD;
            break;
    }

    switch (speed)
    {
        case 2400:
            cfsetispeed(pstTtyattr, B2400);
            cfsetospeed(pstTtyattr, B2400);
            break;
        case 4800:
            cfsetispeed(pstTtyattr, B4800);
            cfsetospeed(pstTtyattr, B4800);
            break;
        case 9600:
            cfsetispeed(pstTtyattr, B9600);
            cfsetospeed(pstTtyattr, B9600);
            break;
        case 57600:
            cfsetispeed(pstTtyattr, B57600);
            cfsetospeed(pstTtyattr, B57600);
            break;
        case 115200:
            //	  cfsetspeed(pstTtyattr,B115200);
            cfsetispeed(pstTtyattr, B115200);
            cfsetospeed(pstTtyattr, B115200);
            break;
        default:
            cfsetispeed(pstTtyattr, B9600);
            cfsetospeed(pstTtyattr, B9600);
            break;
    }
    pstTtyattr->c_cc[VTIME] = 0;
    pstTtyattr->c_cc[VMIN] = 0;
    tcflush(fd, TCIFLUSH);
    if ((tcsetattr(fd, TCSANOW, pstTtyattr)) != 0)
    {
        ;
    }
    else
    {
        err = 0;
    }
    return err;
}

void common_uart_printf(const char *fmt, ...)
{
#ifdef _UART_DEBUG
    va_list va;
    char buff[512];

    if (sg_UartFd == -1)
    {
        UART_DBG("%s", fmt);
        return;
    }
    memset(buff, '\0', 512);
    va_start(va, fmt);
    vsprintf(buff, fmt, va);
    va_end(va);
    write(sg_UartFd, buff, strlen(buff));
    GxCore_ThreadDelay(10);
#endif
}

//#define NORMAL_UART_SUPPORT
#ifdef NORMAL_UART_SUPPORT
#define KERNEL_DEBUG_PRI_CMD1		"echo 3 > /proc/sys/kernel/printk"
#define KERNEL_DEBUG_PRI_CMD2		"echo 7 > /proc/sys/kernel/printk"
#define KERNEL_DEBUG_PRI_CMD3		"rm -rf /dev/console;ln -s /dev/null /dev/console"
#define KERNEL_DEBUG_PRI_CMD4		"rm -rf /dev/console;mknod /dev/console c 5 1"

static int _kernel_printf_pri_adjust(int DiableFlag)
{
    if (DiableFlag)
    {
        system(KERNEL_DEBUG_PRI_CMD1);
        //	system(KERNEL_DEBUG_PRI_CMD3);
    }
    else
    {
        system(KERNEL_DEBUG_PRI_CMD2);
        //	system(KERNEL_DEBUG_PRI_CMD4);
    }
    return 0;
}

static int _user_printf_control(int DisableFlag)
{
    if (DisableFlag)
    {
        int fd = open("/dev/null", O_RDWR);
        if (fd < 0)
        {
            return -1;
        }
        dup2(fd, 0);
        dup2(fd, 1);
        dup2(fd, 2);
        if (fd > 2)
        {
            close(fd);
        }
    }
    else
    {
        // recover
        if (sg_UartFd == -1)
        {
            return -1;
        }
        dup2(sg_UartFd, 0);
        dup2(sg_UartFd, 1);
        dup2(sg_UartFd, 2);
        close(sg_UartFd);
        sg_UartFd = -1;
    }
    return 0;
}

int common_uart_init(unsigned int Baudrate)
{
    int Ret = 0;
    unsigned int BRate = Baudrate;
    struct termios stDefaultTtyattr;

    if (sg_UartFd > 0)
    {
        UART_DBG("\ncommon uart, uart init parameter error\n");
        return -1;
    }
    sg_UartFd = open("/dev/ttyS0", O_RDWR | O_NOCTTY);
    if (sg_UartFd < 0)
    {
        UART_DBG("\ncommon uart, uart device open failed...\n");
        return -1;
    }
    // set baudrate
    tcgetattr(sg_UartFd, &stDefaultTtyattr);
    cfmakeraw(&stDefaultTtyattr);
    set_termios(sg_UartFd, BRate, 8, 1, 'N', &stDefaultTtyattr);

    _kernel_printf_pri_adjust(1);
    _user_printf_control(1);
    return sg_UartFd;
}

int common_uart_receive(unsigned char *Buffer, unsigned int byteToRead)
{
    int RealByteRead = 0;
    int RetLen = 0;
    if ((Buffer == NULL) || (sg_UartFd < 0))
    {
        common_uart_printf("\ncommon uart, uart receive parameter err\n");
        return -1;
    }

    ioctl(sg_UartFd, FIONREAD, &RealByteRead);
    if (RealByteRead > 0)
    {
        RetLen = read(sg_UartFd, Buffer, RealByteRead);
    }

    return RetLen;
}

int common_uart_send(unsigned char *Buffer, unsigned int byteToWrite)
{
    int RealByteGet = 0;
    if ((Buffer == NULL) || (sg_UartFd <= 0))
    {
        return -1;
    }
    RealByteGet = write(sg_UartFd, Buffer, byteToWrite);
    if (RealByteGet != byteToWrite)
    {
        common_uart_printf("\ncommon uart, uart send failed\n");
        return -1;
    }
    return RealByteGet;
}

int common_uart_close(void)
{
    if (sg_UartFd <= 0)
    {
        return -1;
    }
    _kernel_printf_pri_adjust(1);
    _user_printf_control(1);
    return 0;
}
#else

int common_uart_init(unsigned int Baudrate)
{
    struct termios stDefaultTtyattr;

    if (sg_UartFd > 0)
    {
        UART_DBG("\ncommon uart, uart init parameter error\n");
        return -1;
    }
    sg_UartFd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY);
    if (sg_UartFd < 0)
    {
        UART_DBG("\ncommon uart, uart device open failed...\n");
        return -1;
    }
    // set baudrate
    tcgetattr(sg_UartFd, &stDefaultTtyattr);
    cfmakeraw(&stDefaultTtyattr);
    set_termios(sg_UartFd, Baudrate, 8, 1, 'N', &stDefaultTtyattr);

    return sg_UartFd;
}

int common_uart_receive(unsigned char *Buffer, unsigned int byteToRead)
{
    //int RealByteRead = 0;
    int RetLen = 0;
    if ((Buffer == NULL) || (sg_UartFd < 0))
    {
        UART_DBG("\ncommon uart, uart receive parameter err\n");
        return -1;
    }

    //ret = ioctl(sg_UartFd,FIONREAD,&RealByteRead);
    //if(RealByteRead > 0)
    {
        RetLen = read(sg_UartFd, Buffer, byteToRead);
    }
    //else
    //{
    //   app_log_debug("\nret = %d, len = %d\n", ret, RetLen);
    //}

    return RetLen;
}

int common_uart_send(unsigned char *Buffer, unsigned int byteToWrite)
{
    int RealByteGet = 0;
    if ((Buffer == NULL) || (sg_UartFd <= 0))
    {
        return -1;
    }
    RealByteGet = write(sg_UartFd, Buffer, byteToWrite);
    if (RealByteGet != byteToWrite)
    {
        UART_DBG("\ncommon uart, uart send failed\n");
        return -1;
    }
    return RealByteGet;
}

int common_uart_close(void)
{
    if (sg_UartFd <= 0)
    {
        return -1;
    }
    close(sg_UartFd);
    sg_UartFd = -1;
    return 0;
}

#endif
#endif
