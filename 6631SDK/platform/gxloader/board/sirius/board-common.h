#ifndef __BOARD_COMMON_H
#define __BOARD_COMMON_H

#include "stdio.h"
#include "io.h"
#include "util.h"
#include "string.h"
#include "config.h"
#include "partition.h"
#include "gx_gpio.h"
#include "gx_api.h"
#include "boot.h"
#include "gdi.h"

/* DEFAULT VIRTUAL GPIO DEFINE */
#define LOWPOWER_POWERCUT_VIRTUAL_GPIO  201
#define SPI_CS1_VIRTUAL_GPIO            200
#define SPI_CS2_VIRTUAL_GPIO            202
#define DWSPI_CS0_VIRTUAL_GPIO          203

#define ARRAY_END_FLAG                  0xff
#define ARRAY_END_FLAG_U16              0xffff
#define MULPIN_INVALID_VALUE            0xff
#define NOT_CONFIG                      0xff
#define MP_INV_V                        MULPIN_INVALID_VALUE

enum{
	_BOTTOM_,
	_RIGHT_,
	_TOP_,
	_LEFT_
};

enum video_dac_mode{
	DAC_NOT_CONFIG = 0,
	ENABLE_CVBS,
	ENABLE_CVBS_AND_YPBPR
};

enum vout_dac_case {
	CASE_GBRX,//0
	CASE_GBXR,
	CASE_GRBX,
	CASE_GRXB,
	CASE_GXBR,

	CASE_GXRB,//5
	CASE_BGRX,
	CASE_BGXR,
	CASE_BXRG,
	CASE_BXGR,

	CASE_BRXG,//10
	CASE_BRGX,
	CASE_RBGX,
	CASE_RBXG,
	CASE_RGBX,

	CASE_RGXB,//15
	CASE_RXBG,
	CASE_RXGB,
	CASE_XBRG,
	CASE_XBGR,

	CASE_XGRB,//20
	CASE_XGBR,
	CASE_XRBG,
	CASE_XRGB,
};

struct mulpin_config_s{
	unsigned char side;
	unsigned short chip_core_num;
	unsigned char sel0;
	unsigned char sel1;
	unsigned char sel2;
	unsigned char fun;
};

void enable_dac(enum video_dac_mode mode);

void mulpin_init(void);
void gpio_init(void);

void gx_otp_read(unsigned int start_addr, int rd_num, unsigned char *data);
void gx_otp_write(unsigned int start_addr, int wr_num, unsigned char *data);
void gx_otp_status(void);
void gx_otp_lock(void);

/*
note1:  tsx = ts1 or ts2 or ts3
note2:  可能需要配置管脚复用，具体配置咨询硬件
note3:  该接口一般在board-init.c文件中的board_init接口中调用

[ts_port]: TS端口选择，具体配置咨询硬件
1: set ts1
2: set ts2
3: set ts3

[port_mode]: 使用的是串行TS还是并行TS，具体配置咨询硬件
0: parallel mode
1: serial mode

[pin_config]:
当port_mode为0(parallel mode)时，该值可不用关心
当port_mode为1(serial mode)时，对于TSIx中的x代表ts_port，x可以是1/2/3中的任意值，具体配置咨询硬件
mode   function list                 pin list
0:     tsx_serial_in_clk       =     TSIxCLK   ;
       tsx_serial_in_sync      =     TSIxSYNC  ;
       tsx_serial_in_valid     =     TSIxVALID ;
#if TSI1
       tsx_serial_in_dat       =     TSIxDATA5 ;
#else
       tsx_serial_in_dat       =     TSIxDATA0 ;
#endif
1:
       tsx_serial_in_clk       =     TSIxCLK   ;
       tsx_serial_in_sync      =     TSIxSYNC  ;
       tsx_serial_in_valid     =     TSIxVALID ;
       tsx_serial_in_dat       =     TSIxDATA7 ;
2:
       tsx_serial_in_clk       =     TSIxCLK   ;
       tsx_serial_in_sync      =     TSIxDATA7 ;
       tsx_serial_in_valid     =     TSIxDATA6 ;
       tsx_serial_in_dat       =     TSIxDATA5 ;
3:
       tsx_serial_in_clk       =     TSIxCLK   ;
       tsx_serial_in_sync      =     TSIxDATA7 ;
       tsx_serial_in_valid     =     TSIxDATA5 ;
       tsx_serial_in_dat       =     TSIxDATA6 ;
4:
       tsx_serial_in_clk       =     TSIxCLK   ;
       tsx_serial_in_sync      =     TSIxDATA6 ;
       tsx_serial_in_valid     =     TSIxDATA7 ;
       tsx_serial_in_dat       =     TSIxDATA5 ;
5:
       tsx_serial_in_clk       =     TSIxCLK   ;
       tsx_serial_in_sync      =     TSIxDATA6 ;
       tsx_serial_in_valid     =     TSIxDATA5 ;
       tsx_serial_in_dat       =     TSIxDATA7 ;
6:
       tsx_serial_in_clk       =     TSIxCLK   ;
       tsx_serial_in_sync      =     TSIxDATA5 ;
       tsx_serial_in_valid     =     TSIxDATA7 ;
       tsx_serial_in_dat       =     TSIxDATA6 ;
7:
       tsx_serial_in_clk       =     TSIxCLK   ;
       tsx_serial_in_sync      =     TSIxDATA5 ;
       tsx_serial_in_valid     =     TSIxDATA6 ;
       tsx_serial_in_dat       =     TSIxDATA7 ;

[valid_enable]: 具体配置咨询硬件
0: TSIxVALID invalid
1: TSIxVALID valid

[sync_enable]:
当port_mode为0(parallel mode)时，具体配置咨询硬件
当port_mode为1(serial mode)时，固定置1
0: TSIxSYNC invalid
1: TSIxSYNC valid

[endian_select]: 与edge_select一起，由软件进行组合测试，共2*2种
0: little endian
1: big endian
ex. if data=0x47
    little endian & parallel mode: TSIxDATA7=0,D6=1,D5=0,D4=0,D3=0,D2=1,D1=1,D0=1
    big    endian & parallel mode: TSIxDATA7=1,D6=1,D5=1,D4=0,D3=0,D2=0,D1=1,D0=0
    little endian & serial   mode: first bit=1,2nd=1,3rd=1,4th=0,5th=0,6th=0,7th=1,8th=0
    big    endian & serial   mode: first bit=0,2nd=1,3rd=0,4th=0,5th=0,6th=1,7th=1,8th=1

[edge_select]: 见endian说明
0:failing edge
1:rising edge
-1:not config
default->rising edge
 * */
int ts_mode_config(int ts_port, int port_mode, int pin_config, int valid_enable, int sync_enable, int endian_select, int edge_select);

/* for board extend func/data */
typedef int(*func_ex_f)(void *parg);
typedef int(*func_usb_update_f)(void);
struct board_extend_s{
	char *p_ex;
	func_ex_f func_ex;
	func_usb_update_f func_usb_update;
};
extern struct board_extend_s g_board_extend;

void vout_init(void);

int system_config_init(void);

#endif

