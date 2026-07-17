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

#include "app_module.h"
#include <sys/time.h>
#include <sys/select.h>
#include "app_default_params.h"
#include "module/app_smartcard_api.h"



#define CA_TEST
#define SMARTCARD_AUTO_RETRY_COUNT 1 // try  2 times
#define SMARTCARD_MANUAL_RETRY_COUNT 2
#ifndef E_INVALID_HANDLE
#define E_INVALID_HANDLE (0)
#endif

static int ca_thread = 0;
static int ca_thread_control = 0;
static int ca_thread_exit = 0;
static handle_t hsmc;
static int ca_init_success = 0;

/* Return values */
#define ATR_OK					0	/* ATR could be parsed and data returned */
#define ATR_NOT_FOUND			1	/* Data not present in ATR */
#define ATR_MALFORMED			2	/* ATR could not be parsed */

/* Paramenters */
#define ATR_MAX_HISTORICAL		15	/* Maximum number of historical bytes */
#define ATR_MAX_PROTOCOLS		7	/* Maximun number of protocols */
#define ATR_MAX_IB				4	/* Maximum number of interface bytes per protocol */
#define ATR_PROTOCOL_TYPE_T0	0	/* Protocol type T=0 */
#define ATR_INTERFACE_BYTE_TA	0	/* Interface byte TAi */
#define ATR_INTERFACE_BYTE_TB	1	/* Interface byte TBi */
#define ATR_INTERFACE_BYTE_TC	2	/* Interface byte TCi */
#define ATR_INTERFACE_BYTE_TD	3	/* Interface byte TDi */

#define T1_BUFFER_SIZE			(3 + 254 + 2)

/* T1 protocol private*/
typedef struct {
	long		    state;  /*internal state*/
	uint8_t		    ns;	/* reader side  Send sequence number */
	uint8_t		    nr;	/* card side  RCV sequence number*/
	uint32_t		ifsc;
	uint32_t		ifsd;
	uint8_t		    wtx;		/* block waiting time extention*/
	uint32_t		retries;
	uint32_t 		BGT;
	uint32_t 		BWT;
	uint32_t 		CWT;
	uint8_t			more;	/* more data bit */
	uint8_t		    previous_block[4];	/* to store the last R-block */
	uint8_t		    sdata[T1_BUFFER_SIZE];
	uint8_t		    rdata[T1_BUFFER_SIZE];
} t1_state_t;

struct smartcard_private
{
	uint32_t        F;
	uint32_t        D;
	uint32_t        I;
	uint32_t        P;
	uint8_t         N;
	uint8_t         WI;
	uint8_t         T;
	uint8_t         CWI;
	uint8_t         BWI;
	uint8_t         error_check_type;
	uint8_t         TA2_spec;        //for TA(2) bit 5, indicate if to use the default F/D
	uint32_t        smc_clock;
	uint32_t        smc_etu;
	uint32_t        first_cwt;
	uint32_t        cwt;
	t1_state_t      T1;
};

typedef enum
{
    GX_SCARD_PROTOCOL_T0 = 0,
    GX_SCARD_PROTOCOL_T1,
    GX_SCARD_PROTOCOL_INVALID
}GX_SCARD_PROTOCOL;

typedef struct GXSMART_TimeParams_s
{
    uint32_t        Egt;          ///< 当前字节发送完毕后开始发送下一个字节须等待的时钟周期数
                      ///< ,以智能卡时钟周期为单位
    uint32_t        Tgt;          ///< 模块从其它状态转入发送过程时，发送第一个字节前须等待的
                      ///< 时钟周期数，以智能卡时钟周期为单位
    uint32_t        Wdt;          ///< 该值设定了数据接收过程中当前字节起始比特和下一字节起始比
                      ///< 特之间的最大等待时间，以智能卡时钟周期为单位
    uint32_t        Twdt;         ///< 该值设定了模块从其它状态转入接收状态后等待第一个字节到来
                      ///< 的最大等待时间，以智能卡时钟周期为单位
}GXSMART_TimeParams_t;

typedef struct s_ATR
{
  unsigned          length;
  uint8_t           TS;
  uint8_t           T0;

  struct
  {
    uint8_t         value;
    bool            present;
  }ib[ATR_MAX_PROTOCOLS][ATR_MAX_IB], TCK;

  unsigned          pn;
  uint8_t           hb[ATR_MAX_HISTORICAL];
  unsigned          hbn;
  uint32_t          Etu;
  uint32_t          WorkEtu;
  uint32_t          baud_rate;
  uint32_t          card_type;
  GXSMART_TimeParams_t    TimeParams;       ///< 时间参数

}ATR;

#define SMC_VIACCESS_TYPE           0x603
/* Default values for paramenters */
#define ATR_DEFAULT_F	            372
#define ATR_DEFAULT_D	            1
#define ATR_DEFAULT_I 	            50
#define ATR_DEFAULT_N	            0
#define ATR_DEFAULT_P	            5
#define ATR_DEFAULT_WI	            10

#define ATR_DEFAULT_BWI	            4
#define ATR_DEFAULT_CWI	            13

#define ATR_DEFAULT_IFSC	        32
#define ATR_DEFAULT_IFSD	        32
#define ATR_DEFAULT_BGT	            22
#define ATR_DEFAULT_CHK         	0	//default checksum is LRC

#define DFT_WORK_CLK	            3600000//3579545
#define ATR_PROTOCOL_TYPE_T1	    1	/* Protocol type T=1 */
#define DFT_WORK_ETU	            372
#define DEFAULT_BAUD_RATE           (9600 * 372)
#define DEFAULT_ETU	                (372)
#define DEFAULT_EGT	                (DEFAULT_ETU * 12)
#define DEFAULT_WDT	                (9600* DEFAULT_ETU)
#define DEFAULT_TGT	                (0)
#define DEFAULT_TWDT                (0x8FFFFF)

/* defines for ATR bit flags */
#define TA_BIT                      (0x10)
#define TB_BIT                      (0x20)
#define TC_BIT                      (0x40)
#define TD_BIT                      (0x80)

enum {
	IFD_PROTOCOL_RECV_TIMEOUT = 0x0000,
	IFD_PROTOCOL_T1_BLOCKSIZE,
	IFD_PROTOCOL_T1_CHECKSUM_CRC,
	IFD_PROTOCOL_T1_CHECKSUM_LRC,
	IFD_PROTOCOL_T1_IFSC,
	IFD_PROTOCOL_T1_IFSD,
	IFD_PROTOCOL_T1_STATE,
	IFD_PROTOCOL_T1_MORE
};

#define MAX_NUM_TD                  15
#define RFU                         0// 0xFFFF
#define F_RFU	                    0
#define D_RFU	                    0
#define I_RFU		                0
#define ATR_ID_NUM		            16
#define ATR_FI_NUM		            16
#define ATR_DI_NUM		            16
#define ATR_I_NUM		            4

#define MASK_BIT4                   (1 << 4)
#define MASK_BIT5                   (1 << 5)
#define MASK_BIT6                   (1 << 6)
#define MASK_BIT7                   (1 << 7)

#define GET_NUMBER(TD)              ((((TD) >> 4) & 1) + (((TD) >> 5) & 1) +        \
                                     (((TD) >> 6) & 1) + (((TD) >> 7) & 1))

#define INVERT_BYTE(a)		        ((((a) << 7) & 0x80) | \
				                    (((a) << 5) & 0x40) | \
				                    (((a) << 3) & 0x20) | \
				                    (((a) << 1) & 0x10) | \
				                    (((a) >> 1) & 0x08) | \
				                    (((a) >> 3) & 0x04) | \
				                    (((a) >> 5) & 0x02) | \
				                    (((a) >> 7) & 0x01))



void thread_smc(void* arg)
{
    int ret = -1,i = 0;
    int autoretry = 0;
    GxSmcCardStatus status = GXSMC_CARD_INIT;
    GxSmcCardStatus status_old = GXSMC_CARD_INIT;
    size_t atr_len;
    uint8_t atr[256];
    ca_thread_exit = 0;
    GxSmcParams param;

    param.detect_pole = GXSMC_DETECT_HIGH_LEVEL;
    //param.detect_pole = GXSMC_DETECT_LOW_LEVEL;
    param.io_conv = GXSMC_DATA_CONV_DIRECT;
    param.parity = GXSMC_PARITY_EVEN;
    //param.parity = GXSMC_PARITY_ODD;
    param.protocol = DISABLE_REPEAT_WHEN_ERR;
    param.sci_sel = __GXSMART_SCI1;
    param.stop_len = GXSMC_STOPLEN_0BIT;
#ifdef SIRIUS
    param.vcc_pole = GXSMC_VCC_LOW_LEVEL;
#else
    param.vcc_pole = GXSMC_VCC_HIGH_LEVEL;
#endif
    param.default_etu = 625;
    param.auto_etu = 1;
    param.auto_parity = 1;
    //param.debug_info = O_BDBG | O_CDBG;

    hsmc = GxSmc_Open(NULL, &param);
    if (E_INVALID_HANDLE == hsmc)
    {
        app_log_min("\nSmartCard, open smc err!\n");
        return;
    }
start:
    while(ca_thread_control)
    {
        GxSmc_GetStatus(hsmc,&status);
        if ((status == GXSMC_CARD_IN) && (status_old != GXSMC_CARD_IN))
        {
            atr_len = 0;
            memset(atr,0,256);
            app_log_min("\nSmartCard, a smart card inserterd >>>>>>>>>>>>>>>\n");
            ret = GxSmc_Reset(hsmc,atr,255,&atr_len);
            if((atr_len == 0) ||(ret < 0))
            {
                app_log_min("\nSmartCard, ATR error: len = %d, ret = %d\n",atr_len,ret);
                if(autoretry++ >= SMARTCARD_AUTO_RETRY_COUNT)
                {
                    goto changemode;
                }
                else
                {
                    GxCore_ThreadDelay(100);
                    continue;
                }
            }
            app_log_min("SmartCard, get atr,len = %d : Err:%d\n",atr_len,ret);
            for( i=0 ; i<atr_len ; i++ )
            {
                app_log_min(" %x,",atr[i]);
            }
            app_log_min("\n");
            app_log_min("SmartCard, Start analyse the ATR!!\n");
            GxSciTimeParams time = {0};

            if(GxSci_AnalyseAtr(atr, atr_len, &time) < 0)
            {
                if(autoretry++ >= SMARTCARD_AUTO_RETRY_COUNT)
                {
                    goto changemode;
                }
                else
                {
                    GxCore_ThreadDelay(200);//(2000);
                    continue;
                }
            }

            GxSmcConfigs ConfigP = {0};
            ConfigP.flags = SMCC_TIME;
            ConfigP.time.flags = SMCT_EGT|SMCT_ETU|SMCT_TGT|SMCT_TWDT|SMCT_WDT;//SMCT_ALL;
            ConfigP.time.baud_rate = time.clock;
            ConfigP.time.egt = time.egt;
            ConfigP.time.tgt = time.tgt;
            ConfigP.time.twdt = time.twdt;
            ConfigP.time.wdt = time.wdt;
            ConfigP.time.etu = time.etu;
            app_log_min("\nSMART CARD, bud = %d, work etu = %d\n", ConfigP.time.baud_rate, ConfigP.time.etu);
            app_log_min("\nbud = %d, egt = %d,tgt = %d,twdt = %d,wdt = %d,etu = %d\n",ConfigP.time.baud_rate,ConfigP.time.egt,ConfigP.time.tgt,ConfigP.time.twdt,ConfigP.time.wdt,ConfigP.time.etu);

            GxSmc_ConfigAll(hsmc,&ConfigP);
            ca_init_success = 1;
            app_log_min("\nSmartCard, successfully!!!\n");
        }
        else if((status ==GXSMC_CARD_OUT) && (status_old == GXSMC_CARD_IN))
        {
            app_log_min("\nSmartCard,--card remove!!!\n");
            ca_init_success = 0;
            {
                GxSmcConfigs ConfigP = {0};

                ConfigP.flags = SMCC_AUTO_ENABLE|SMCC_AUTO_ETU_ENABLE|SMCC_AUTO_CONV_PARITY_ENABLE;
                GxSmc_ConfigAll(hsmc,&ConfigP);
            }
            autoretry = 0;
        }
        status_old = status;
        GxCore_ThreadDelay(200);//(2000);
    }
    ca_thread_exit = 1;
    return;
changemode:
    app_log_min("\nSMART CARD, change manual mode, flag = %d\n", (autoretry-SMARTCARD_AUTO_RETRY_COUNT)%2);
    if(((autoretry-SMARTCARD_AUTO_RETRY_COUNT)%2)!=0)
    {
        GxSmcConfigs ConfigP = {0};

        ConfigP.flags = SMCC_AUTO_DISABLE|SMCC_AUTO_ETU_DISABLE|SMCC_AUTO_CONV_PARITY_ENABLE|SMCC_TIME;
        ConfigP.time.flags = SMCT_ETU;
        ConfigP.time.etu = 625;
        GxSmc_ConfigAll(hsmc,&ConfigP);
    }
    else
    {
        GxSmcConfigs ConfigP = {0};

        ConfigP.flags = SMCC_AUTO_DISABLE|SMCC_AUTO_ETU_DISABLE|SMCC_AUTO_CONV_PARITY_ENABLE|SMCC_TIME;
        ConfigP.time.flags = SMCT_ETU;
        ConfigP.time.etu = 372;
        GxSmc_ConfigAll(hsmc,&ConfigP);

    }
    goto start;
}

void start_ca_test(void)
{
    if(ca_thread == 0)
    {
        ca_thread_control = 1;
        GxCore_ThreadCreate("ca_test", &ca_thread, thread_smc, NULL,100*1024,GXOS_DEFAULT_PRIORITY);
    }
    else
    {
        app_log_min("\nSmartCard, smart card thread had created\n");
    }
}

void distory_ca_test(void)
{
    if(ca_thread)
    {
        ca_thread_control = 0;
        while(ca_thread_exit != 0)
        {
            GxCore_ThreadDelay(100);
        }

        if(hsmc != E_INVALID_HANDLE)
        {
            GxSmc_Close(hsmc);
            hsmc = E_INVALID_HANDLE;
        }
        GxCore_ThreadJoin(ca_thread);
        ca_thread = 0;
    }
}
