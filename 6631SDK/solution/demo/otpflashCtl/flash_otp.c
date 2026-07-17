#include <string.h>
#include "app.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#define FALSH_OTP_DEBUG  (1)

/* Spi Flash OTP function */
#ifdef ECOS_OS
#define SET_CONFIG_FLASH_OTP_LOCK          (0x610)
#define GET_CONFIG_FLASH_OTP_STATUS        (0x611)
#define SET_CONFIG_FLASH_OTP_ERASE         (0x612)
#define GET_CONFIG_FLASH_OTP_GET_REGION    (0x613)
#define SET_CONFIG_FLASH_OTP_SET_REGION    (0x614)
#define GET_CONFIG_FLASH_OTP_READ          (0x615)
#define SET_CONFIG_FLASH_OTP_WRITE         (0x616)
#define DEVNAME "/dev/flash/0/0x0"

#else

#define SET_CONFIG_FLASH_OTP_LOCK           (0x2000)
#define GET_CONFIG_FLASH_OTP_STATUS         (0x3000)
#define SET_CONFIG_FLASH_OTP_ERASE          (0x4000)
#define GET_CONFIG_FLASH_OTP_GET_REGION     (0x5000)
#define SET_CONFIG_FLASH_OTP_SET_REGION     (0x6000)
#define DEVNAME "/dev/flash_otp"
#endif

//#define FLASH_OTP_API_TEST

static int fd = 0;

#ifdef ECOS_OS
typedef struct {
    unsigned int addr;
    unsigned char *buf;
    size_t count;
}flashOtpWRInfo_t;

static int sflash_otp_read(unsigned int addr, unsigned char* data, unsigned int len)
{
	int  err  = 0;
	flashOtpWRInfo_t info;

	info.addr = addr;
	info.count = len;
	info.buf = GxCore_Malloc(info.count);
	if (info.buf == NULL)
		return -1;

	err = ioctl(fd, GET_CONFIG_FLASH_OTP_READ, &info);
	if (err != 0) {
		app_log_error("otp read fail, ret = 0x%x\n", err);
		return -1;
	}
	app_log_min("otp read ok.\n");

    memcpy(data, info.buf, len);
#if FALSH_OTP_DEBUG
    app_log_min("Read... ");
	int i, j;
	for(i = info.addr, j = 0; i < info.addr+info.count; i++, j++) {
		if (i%16==0)
			app_log_min("\n%8.8x", info.addr+j);
		if (i%2==0)
			app_log_min(" ");
		app_log_min("%2.2x", info.buf[j]);
	}
	app_log_min("\n");
#endif

    if(info.buf != NULL)
    {
        GxCore_Free(info.buf);
        info.buf = NULL;
    }
	return err;
}

static int sflash_otp_write(unsigned int addr, unsigned char *data, unsigned int len)
{
	int  err  = 0;
	flashOtpWRInfo_t info;

    if(data ==NULL)
        return -1;

    info.addr = addr;
    info.count = len;
    info.buf = data;

#if FALSH_OTP_DEBUG
	int i, j;
    app_log_min("Write... ");
	for(i = info.addr, j = 0; i < info.addr+info.count; i++, j++)
    {
		if (i%16==0)
			app_log_min("\n%8.8x", info.addr+j);
		if (i%2==0)
			app_log_min(" ");
		app_log_min("%2.2x", info.buf[j]);
	}
	app_log_min("\n");
#endif

	err = ioctl(fd, SET_CONFIG_FLASH_OTP_WRITE, &info);
	if (err != 0)
    {
		app_log_error("otp write fail, ret = 0x%x\n", err);
		return -1;
	}
	app_log_min("otp write ok.\n");

	return err;
}

#else
static int get_otp_size(void)
{
	int pos;
#ifdef FLASH_OTP_API_TEST
	pos = GxFlashOtp_GetSize();
#else
	pos=lseek(fd,0,SEEK_END);
#endif
	if (pos < 0)
	{
		app_log_error("error:get flash otp size! \n");
	}
	else
	{
		app_log_min("flash otp size is %d byte\n",pos);
	}
	return pos;
}

static int sflash_otp_read(unsigned int addr, unsigned char* data, unsigned int len)
{
	int done;
	int i;
	unsigned char buf[1024] = {0};
#ifdef FLASH_OTP_API_TEST
	done = GxFlashOtp_Read(addr, len, buf);
#else
	lseek(fd,addr,SEEK_SET);
	done = read( fd, buf,len);
    memcpy(data, buf, len);
#endif
	if (done < 0)
	{
		app_log_error("otp read no data \n");
	}
	else
	{
		app_log_min("otp read success.\n");
#if FALSH_OTP_DEBUG
		app_log_min("otp read data:\n");
		for(i=0; i<done; i++)
		{
			app_log_min(" 0x%02x", data[i]);
			if(0==(i+1)%10)
				app_log_min("\n");
		}
		app_log_min("\n");
#endif
	}
	return 0;
}

static int sflash_otp_write(unsigned int addr, unsigned char *data, unsigned int len)
{
	int done;
	int i;

#ifdef FLASH_OTP_API_TEST
	done = GxFlashOtp_Write(addr, len, data);
#else
	lseek(fd,addr,SEEK_SET);
	done = write(fd, data,len);
#endif
	if (done < 0)
	{
		app_log_error("otp write data error\n");
	}
	else
	{
		app_log_min("otp write success.\n");
#if FALSH_OTP_DEBUG
		app_log_min("otp write data:\n");
		for(i=0; i<done; i++)
		{
			app_log_min(" 0x%02x", data[i]);
			if(0==(i+1)%10)
				app_log_min("\n");
		}
		app_log_min("\n");
#endif
	}
	return 0;
}
#endif

int sflash_otp_lock(void)
{
	int  err  = 0;

#ifdef FLASH_OTP_API_TEST
	err = GxFlashOtp_Lock();
#else
	err = ioctl(fd, SET_CONFIG_FLASH_OTP_LOCK, NULL);
#endif
	if (err == 0) {
		app_log_min("otp lock ok\n");
	} else {
		app_log_error("otp fail, ret = %d\n", err);
	}

	return err;
}

int sflash_otp_status(unsigned char *buf)
{
	int  err  = 0;

#ifdef FLASH_OTP_API_TEST
		err = GxFlashOtp_Status(buf);
#else
	err = ioctl(fd, GET_CONFIG_FLASH_OTP_STATUS, buf);
#endif
	if (err == 0) {
		app_log_min("otp status = 0x%x\n", *buf);
	} else {
		app_log_error("get otp status fail, ret = %d\n", err);
	}

	return err;
}

int sflash_otp_get_region_num(unsigned int *num)
{
	int  err  = 0;

#ifdef FLASH_OTP_API_TEST
	err = GxFlashOtp_GetRegion(num);
#else
	err = ioctl(fd, GET_CONFIG_FLASH_OTP_GET_REGION, num);
#endif
	if (err == 0) {
		app_log_min("otp region num = 0x%x\n", *num);
	} else {
		app_log_error("get otp region num fail, ret = %d\n", err);
	}

	return err;
}

int sflash_otp_set_region(unsigned int region)
{
	int  err  = 0;

#ifdef FLASH_OTP_API_TEST
	err = GxFlashOtp_SetRegion(&region);
#else
	err = ioctl(fd, SET_CONFIG_FLASH_OTP_SET_REGION, &region);
#endif
	if (err == 0) {
		app_log_min("otp set region finish, switch to region %d.\n", region);
	} else {
		app_log_error("otp set region fail, ret = %d\n", err);
	}

	return err;
}

int sflash_otp_erase(void)
{
	int  err  = 0;

#ifdef FLASH_OTP_API_TEST
	err = GxFlashOtp_Erase();
#else
	err = ioctl(fd, SET_CONFIG_FLASH_OTP_ERASE, NULL);
#endif
	if (err == 0) {
		app_log_min("otp erase ok.\n");
	} else {
	    app_log_error("otp erase fail, ret = %d\n", err);
	}

	return err;
}


int sflash_otp_init(void)
{
#ifdef FLASH_OTP_API_TEST
	app_log_min("---------- FLASH OTP API TEST --------\n");
	return GxFlashOtp_Open();
#else
	fd = open(DEVNAME, O_RDWR);
	if( fd < 0 )
	{
		app_log_error("ERROR:open %s faile \n",DEVNAME);
		return fd;
	}
	return 0;
#endif
}


int app_flash_otp_test(void)
{
    int i = 0;
	int ret = 0;
    unsigned int num = 0;
    unsigned char lock_status = -1;

    unsigned int addr = 0;
    unsigned char str[3][16] = {"abcdefg", "1234567", "ABCDEFG"};
    unsigned char buf[1024] = {0};

	if(sflash_otp_init() < 0)
	{
		return -1;
	}
	app_log_min("Open flash otp success.\n");
#ifdef LINUX_OS
	int len = 0;
	if((len = get_otp_size()) < 0)
	{
		return -1;
	}
	sflash_otp_read(addr, buf, len);
#endif
    if((ret = sflash_otp_get_region_num(&num)) < 0) return -1;
    for(i=0; i<num; i++)
    {
        app_log_min(" -- region: %d --\n", i);
        if((ret = sflash_otp_set_region(i)) < 0) break;
        if((ret = sflash_otp_status(&lock_status)) < 0) break;
        if(lock_status == 0)
        {
            if((ret = sflash_otp_erase()) < 0) return -1;
            sflash_otp_read(addr, buf, 16);
            sflash_otp_write(addr, str[i%3], 16);
            sflash_otp_read(addr, buf, 16);
            if(i == 0)
            {
                //sflash_otp_lock();
                sflash_otp_status(&lock_status);
            }
        }
    }

	close(fd);
    return 0;
}
