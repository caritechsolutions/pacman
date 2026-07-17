/*
 * =====================================================================================
 *
 *       Filename:  app_test.c
 *
 *       Description:  Factory Test
 *
 *        Version:  1.0
 *        Created:  02/14/2012 01:18:15 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */


#ifdef ECOS_OS
#include "app.h"
#include "app_module.h"
#include <sys/time.h>
#include <sys/select.h>
#include "app_default_params.h"
 #include "app_factory_test_date.h"
#include "app_windows.h"

#if FACTORY_TEST_SUPPORT
#include "network.h"
#include "dhcp.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <pkgconf/libc_string.h>


int uart_thread[2] = {0};
int ca_thread[2] = {0};
int sg_NetworkTaskThread[2] = {0};

#ifdef GX3200
#define GET_CHIPID
#endif

#ifdef  GET_CHIPID
void test_chip_id(void)
{
	int cs,byte_addr,onebyte,ctrl_word,i;
	unsigned char otp[32] = {0};

	app_log_debug("chip_id:\n");
	cs = 5;
	//for(cs = 5; cs<6; cs++)
	{
		app_log_debug("\n CURSTOM ID = ");
		for(byte_addr=0; byte_addr<8; byte_addr++)
		{
			ctrl_word = ((cs) << 14) | (byte_addr) | (1<<10) | (1<<12);
			*(volatile unsigned int*)0xd050A194 = ctrl_word;
			i=1000;while(i--);
			*(volatile unsigned int*)0xd050A194 = ctrl_word | (1<<11);
			i=1000;while(i--);
			*(volatile unsigned int*)0xd050A194 = ctrl_word;

			onebyte = (*(volatile unsigned int*)0xd050A190)&0xff;
			otp[byte_addr] = onebyte;
			app_log_debug("0x%02x ",onebyte);
			if( (byte_addr & 0xf) == 0xf )
				app_log_debug("\n");
		}
	    app_log_debug("\n CHIP ID = ");
		for(byte_addr=16; byte_addr<24; byte_addr++)
		{
			ctrl_word = ((cs) << 14) | (byte_addr) | (1<<10) | (1<<12);
			*(volatile unsigned int*)0xd050A194 = ctrl_word;
			i=1000;while(i--);
			*(volatile unsigned int*)0xd050A194 = ctrl_word | (1<<11);
			i=1000;while(i--);
			*(volatile unsigned int*)0xd050A194 = ctrl_word;

			onebyte = (*(volatile unsigned int*)0xd050A190)&0xff;
			otp[byte_addr] = onebyte;
			app_log_debug("0x%02x ",onebyte);
			if( (byte_addr & 0xf) == 0xf )
				app_log_debug("\n");
		}
	}
	*(volatile unsigned int*)0xd050A194 = 0;
	app_log_debug("\n");
}
#endif


#define TEST_UART

static int serial_fd;

static void UartT_ConfigUart(void)
{
	#define O_NONBLOCK_    04000U
	unsigned int baud_value = 0;
	int err;

	//close uart printf
	GxCore_PrintSwitch(false);
	serial_fd = open("/dev/ser0",O_RDWR|O_NONBLOCK_);//2015

	GxBus_ConfigGetInt(UART_BAUD_KEY, (int32_t*)&baud_value, UART_BAUD_RATE_LIMIT);
	if(1 == baud_value)
		baud_value = 0xFFFFFFFF;// set 0xd040000c = 0xd, about, 120535
	else
		baud_value = 115200;// set 0xd040000c = 0xe, about,112500

	err=ioctl(serial_fd,0x0191/*CYG_IO_SET_CONFIG_SERIAL_BAUD_RATE*/, (void*)&baud_value);
	if((serial_fd  < 0) || (err < 0))
	{
		return;
	}
}

void UartT_main(void* args)
{
	unsigned long 		writeSize=0, readSize=0;
	unsigned char 	WriteBuffer[10]={0};
	unsigned char 	ReadBuffer[10]={0};
	//volatile unsigned long time_out = 10;
	uint32_t iIndex = 1;
	//app_log_debug("g debug :FUNC:%s:LINE:%d\n",__FUNCTION__,__LINE__);
	UartT_ConfigUart();

	WriteBuffer[0] = 0x55;//U
	WriteBuffer[1] = 0x61;//a
	WriteBuffer[2] = 0x72;//r
	WriteBuffer[3] = 0x74;//t
	WriteBuffer[4] = 0x20;//
	WriteBuffer[5] = 0x69;//i
	WriteBuffer[6] = 0x73;//s
	WriteBuffer[7] = 0x20;//
	WriteBuffer[8] = 0x4f;//O
	WriteBuffer[9] = 0x4b;//K
	while (uart_thread[ThreadFlag])
		{
	writeSize = 10;
	write(serial_fd,WriteBuffer,writeSize);

	readSize = 10;
	//while(time_out--)
	//{
		memset(ReadBuffer,0,10*sizeof(char));
		read(serial_fd,ReadBuffer,readSize);
		if(ReadBuffer[0] ==0x55)
		{
			//GUI_SetProperty("edit_factory_test_opt1", "string", "OK");
			iIndex = 1;
			GUI_SetProperty("edit_factory_test_opt1", "select", &iIndex);
			//break;
		}
		else
		{
			//GUI_SetProperty("edit_factory_test_opt1", "string", "None");
			iIndex = 0;
			GUI_SetProperty("edit_factory_test_opt1", "select", &iIndex);
		}
		GxCore_ThreadDelay(500);//(2000);
	}
	//测试完毕，关闭串口
	close(serial_fd);
	serial_fd = -1;
	GxCore_PrintSwitch(true);
	return ;
}

void uartshort_thread(void)
{
	//static int uart_flag = 0;
	uart_thread[ThreadFlag] = 0;
	GxCore_ThreadCreate("uartshort_test",&uart_thread[ThreadHandle],UartT_main,NULL,100*1024,GXOS_DEFAULT_PRIORITY);
	if(uart_thread[ThreadHandle])
		uart_thread[ThreadFlag] = 1;
}

void destroy_thread(int * pthread_id)
{
  //  app_log_debug("g debug :FUNC:%s:line:%d:thread_id :%d\n",__FUNCTION__,__LINE__,pthread_id[0]);
    if(NULL !=(int *)pthread_id[ThreadHandle])
   	{
   		pthread_id[ThreadFlag] =NullThread;
   		//app_log_debug("g debug :FUNC:%s:line:%d:thread_id :%d\n",__FUNCTION__,__LINE__,pthread_id[1]);
		GxCore_ThreadJoin(pthread_id[ThreadHandle]);
		pthread_id[ThreadHandle] =NullThread;
    }
}

#ifdef GX3200
#define LNB_TEST
#endif

#ifdef  LNB_TEST
//#define A8293
unsigned char LnbTest_main(void)
{
	unsigned char		Ret = 0;
	uint32_t value;
	volatile unsigned long time_out = 200;
	int32_t tuner = 0;
	uint8_t f[32] = {0};
	handle_t    frontend;

    app_log_flow("\n====LNB Tset Start!====\n");
	//Test Tuner1
	tuner = 0;
	sprintf((char*)f,"/dev/dvb/adapter0/frontend%d",tuner);
	frontend = open((char*)f,O_RDWR);
	ioctl (frontend, FE_SET_VOLTAGE, SEC_VOLTAGE_18);

	//GPIO Init GPIO66
	*(volatile unsigned int*)0xd050a134 &= ~(0x1<<9);
	*(volatile unsigned int*)0xd050a134 |= (0x1<<8);//Set Port66 as GPIO
	//Input
	*(volatile unsigned int*)0xd0507000 &= ~(0x1<<2);
#ifdef A8293
	void* I2cA = NULL;
	void* I2cB = NULL;
	I2cA = _A8293_i2c_open(0);
	if(I2cA == NULL)
		app_log_flow("\n[%s]OPEN I2CA DEVICE FAILED\n",__FUNCTION__);
	I2cB = _A8293_i2c_open(0);
	if(I2cB == NULL)
		app_log_flow("\n[%s]OPEN I2CB DEVICE FAILED\n",__FUNCTION__);

#endif
	while(time_out--)
	{
		GxCore_ThreadDelay(200);
		//value 1:Normal 0:short circuit
		value = (*(volatile uint32_t*)0xd0507004 >>2) & 1;
		app_log_flow("\n[%03d]===Lnb Test Tuner1:%d===\n",(int32_t)time_out,value);
		if(value == 0)
		{
#ifdef A8293
			if(_A8293_status(TUNER1_A8293_ADDRESS,I2cA) != 1)
				continue;
			else
#endif
			Ret = 1;// T1短路
			app_log_flow("\n T1 short cut\n");
			break;
		}
	}
	close(frontend);

	//Test Tuner2
	tuner = 1;
	time_out = 200;
	sprintf((char*)f,"/dev/dvb/adapter0/frontend%d",tuner);
	frontend = open((char*)f,O_RDWR);
	ioctl (frontend, FE_SET_VOLTAGE, SEC_VOLTAGE_18);

	//GPIO Init GPIO32
	*(volatile unsigned int*)0xd050a130 |= (0x3<<2);//Set Port32 as GPIO
	//Input
	*(volatile unsigned int*)0xd0506000 &= ~(0x1<<0);
	while(time_out--)
	{
		GxCore_ThreadDelay(200);
		//value 1:Normal 0:short circuit
		value = (*(volatile uint32_t*)0xd0506004 >>0) & 1;
		app_log_flow("\n[%03d]===Lnb Test Tuner2:%d===\n",(int32_t)time_out,value);
		if(value == 0)
		{
#ifdef A8293
			if(_A8293_status(TUNER2_A8293_ADDRESS,I2cB) != 1)
				continue;
			else
#endif
			Ret = 2;//T2短路
			app_log_flow("\n T2 short cut\n");
			break;
		}
	}
	close(frontend);

	return Ret;
}
#endif

#ifdef  FACTORY_TEST_NETWORK_SUPPORT
// 目前网络部分实现还有问题，后续测试稳定提供。
#define TEST_NETWORK

#define NETWORK_TASK_STACK_SIZE		(200*1024)
#define NETWORK_TASK_PRIORITY			GXOS_DEFAULT_PRIORITY//7 //5TODO: temp, please replace it with your own PRIORITY

//static int sg_NetworkTaskThread = 0;
struct  in_addr DNSSERVER[4];
static unsigned int DNSSERVERNUM = 0;


static void get_dhcp_dns(struct  in_addr *dns,unsigned int *num)
{
	int i, len;
	struct bootp *bp = &eth0_bootp_data;
	unsigned char *op, *ap = 0, optover;

	unsigned char name[128];
	struct in_addr addr[32];
	unsigned int length;

	optover = 0; // See whether sname and file are overridden for options
	length = sizeof(optover);
	*num = 0;
	app_log_flow("  in my get_dhcp_info functuin .......................\n");
	if (bp->bp_vend[0])
	{
		app_log_flow("  options:\n");
		op = &bp->bp_vend[4];
		while (*op != TAG_END)
		{
			switch (*op)
			{
				case TAG_DOMAIN_SERVER:
					ap = (unsigned char *)&addr[0];
					len = *(op+1);
					for (i = 0;  i < len;  i++)
					{
						*ap++ = *(op+i+2);
					}
					if (*op == TAG_DOMAIN_SERVER)
					{
						ap = (unsigned char *) "domain server";
						app_log_flow("      %s: ", ap);
						ap = (unsigned char *)&addr[0];
						while (len > 0)
						{
							app_log_flow("%s", inet_ntoa(*(struct in_addr *)ap));
							memcpy(dns + (*num),ap,sizeof(struct in_addr));
							len -= sizeof(struct in_addr);
							ap += sizeof(struct in_addr);
							if (len)
								app_log_flow(", ");
						}
						(*num) ++;
						if(*num>3)
							break; //max support 4 dns_addr;
					}
					app_log_flow("\n");
					break;
				case TAG_DOMAIN_NAME:
				case TAG_HOST_NAME:
					for (i = 0;  i < *(op+1);  i++)
					{
						name[i] = *(op+i+2);
					}
					name[*(op+1)] = '\0';
					if (*op == TAG_DOMAIN_NAME)
						ap = (unsigned char *) " domain name";
					if (*op == TAG_HOST_NAME)
						ap = (unsigned char *) "   host name";
					app_log_flow("       %s: %s\n", ap, name);
					break;
				default:
					break;
			}
			op += *(op+1)+2;
		}
	}

}

 int network_check_cable_link(void)
{
	int i = 10,b = 0,c = 0;
	while(i--)
	{
		if(0 == synopGMAC_get_link())//if(0!=network_cable_link())
		{
			b++;
		}
		else
		{
			c++;
			//b = 0;
		}
	}
	if(b >= c)
		b = 0 ;
	else
		b = 1;
	return b;// 0:unlink; 1:link
}


static int DHCPState =0;
static int network_init_dhcp(void)
{
	static char cIP_STR[17]={"0.0.0.0"};

	if( network_check_cable_link() && DHCPState == 0 )
	{
		//app_log_debug("\n[YES]network_init_dhcp()->Cable linked!!!!\n");
		dhcp_halt();
		//app_log_debug(" debug:%s:line:%d:\n",__FUNCTION__,__LINE__);
		init_all_network_interfaces(); // 如果拔出网线，会一直检测
		//app_log_debug("g debug:%s:line:%d:\n",__FUNCTION__,__LINE__);

		if(eth0_up)
		{
			 get_dhcp_dns(DNSSERVER,&DNSSERVERNUM);
			 //show_bootp("eth0",&eth0_bootp_data);
			 sprintf(cIP_STR,"%d.%d.%d.%d",\
			  eth0_bootp_data.bp_yiaddr.s_addr & 0x000000ff,\
			 (eth0_bootp_data.bp_yiaddr.s_addr >> 8) &0x000000ff,\
			 (eth0_bootp_data.bp_yiaddr.s_addr >> 16) &0x000000ff,\
			 (eth0_bootp_data.bp_yiaddr.s_addr >> 24) &0x000000ff);
			// app_log_debug("g debug:%s:line:%d:ADDR:%s\n",__FUNCTION__,__LINE__,cIP_STR);
			 if( GUI_CheckDialog(WND_FACTORY_TEST) == GXCORE_SUCCESS)
				 GUI_SetProperty(IP_ADDR,"string",&cIP_STR);
			 DHCPState = 1;
			 return 0;
		}
		else
		{
			memset(DNSSERVER,0,sizeof(struct in_addr)*4);
			DNSSERVERNUM = 0;
			//app_log_debug("g debug:%s:line:%d:ADDR:%s\n",__FUNCTION__,__LINE__,cIP_STR);
			//GUI_SetProperty(IP_ADDR,"string","0.0.0.0");
			//return -1;
		}
		//return 0;
	}
	else if ( 0 == network_check_cable_link() )
	{
		if( GUI_CheckDialog("wnd_factorytest") == GXCORE_SUCCESS)
			GUI_SetProperty(IP_ADDR,"string","0.0.0.0");
		DHCPState = 0;
		//app_log_debug("\n[NO]network_init_dhcp()->no cable linked!!!!\n");
	}
	return -1;
}


static void network_thread(void *arg)
{
	while(sg_NetworkTaskThread[ThreadFlag])
	{
		//app_log_debug("g debug :FUNC:%s:line:%d:thread_id :%d\n",__FUNCTION__,__LINE__,sg_NetworkTaskThread[1]);
		if(sg_NetworkTaskThread[ThreadFlag] == EnableThread)
		{
			//network_execute();
			//app_log_debug("g debug :FUNC:%s:line:%d:thread_id :%d\n",__FUNCTION__,__LINE__,sg_NetworkTaskThread[0]);
			network_init_dhcp();
			//sg_NetworkTaskThread[ThreadFlag] =3; // 标记执行完成
			//app_log_debug("g debug :FUNC:%s:line:%d:thread_id :%d\n",__FUNCTION__,__LINE__,sg_NetworkTaskThread[0]);
		}
		else if(sg_NetworkTaskThread[ThreadFlag] ==WaitThread)
		{
			sg_NetworkTaskThread[ThreadFlag] =DisableThread;
			//app_log_debug("g debug :FUNC:%s:line:%d:thread_id :%d\n",__FUNCTION__,__LINE__,sg_NetworkTaskThread[0]);
		}
		GxCore_ThreadDelay(200);
	}
}

void app_network_module_init(void)
{
	sg_NetworkTaskThread[ThreadFlag] = NullThread;
	DHCPState = 0 ;
	//app_log_debug("g debug:func:%s:line:%d:\n",__FUNCTION__,__LINE__);
	GxCore_ThreadCreate("net_app", &sg_NetworkTaskThread[ThreadHandle], (void*)network_thread, NULL,\
		NETWORK_TASK_STACK_SIZE,NETWORK_TASK_PRIORITY);
	//app_log_debug("g debug:func:%s:line:%d:sg_NetworkTaskThread:%d\n",__FUNCTION__,__LINE__,sg_NetworkTaskThread[ThreadHandle]);
	if(sg_NetworkTaskThread[ThreadHandle])
		sg_NetworkTaskThread[ThreadFlag] = EnableThread;

	return;
}
#endif
//void app_network_thread_destroy(void)
//{
//	ThreadFlag = 0;
//	GxCore_ThreadJoin(sg_NetworkTaskThread);
//	sg_NetworkTaskThread = 0;

//}

void app_factory_scart_gpio_init(void)
{
	return;
#ifdef ECOS_OS
	// port 76 for mute control; port 65 for CVBS/RGB[1:CVBS;0:RGB];port 64 &port 67 for 4:3/16:9[(1,0):16:9; (0,0):4:3]
	// Multi-purpose
	*(volatile unsigned int*)0xd050a134 &= ~(1<<9);
    *(volatile unsigned int*)0xd050a134 |= (1<<8);
#endif
}
// mute = 0: enable the sound output; mute = 1: disable the sound output
void app_factory_scart_mute_control(int32_t mute)
{
	return;
// GPIO 76
#ifdef ECOS_OS
	// set for output
	*(volatile unsigned int*)0xd0507000 |= (1<<12);

	if(mute == 0)
	{// set 0 for output
		*(volatile unsigned int*)0xd050700c |= (1<<12);
	}
	else
	{// set 1 for output
		*(volatile unsigned int*)0xd0507008 |= (1<<12);
	}
#endif
}
//
#endif

#endif

