#include "gxcore.h"
#include "../../include/gxcas_smartcard.h"
/* Return values */
#define ATR_OK                  0       /* ATR could be parsed and data returned */
#define ATR_NOT_FOUND           1       /* Data not present in ATR */
#define ATR_MALFORMED           2       /* ATR could not be parsed */

/* Paramenters */
#define ATR_MAX_HISTORICAL      15      /* Maximum number of historical bytes */
#define ATR_MAX_PROTOCOLS       7       /* Maximun number of protocols */
#define ATR_MAX_IB              4       /* Maximum number of interface bytes per protocol */
#define ATR_PROTOCOL_TYPE_T0    0       /* Protocol type T=0 */
#define ATR_INTERFACE_BYTE_TA   0       /* Interface byte TAi */
#define ATR_INTERFACE_BYTE_TB   1       /* Interface byte TBi */
#define ATR_INTERFACE_BYTE_TC   2       /* Interface byte TCi */
#define ATR_INTERFACE_BYTE_TD   3       /* Interface byte TDi */

#define T1_BUFFER_SIZE          (3 + 254 + 2)

/* T1 protocol private*/
typedef struct {
	long                state;  /*internal state*/
	uint8_t             ns; /* reader side  Send sequence number */
	uint8_t             nr; /* card side  RCV sequence number*/
	uint32_t            ifsc;
	uint32_t            ifsd;
	uint8_t             wtx;                /* block waiting time extention*/
	uint32_t            retries;
	uint32_t            BGT;
	uint32_t            BWT;
	uint32_t            CWT;
	uint8_t             more;   /* more data bit */
	uint8_t             previous_block[4];  /* to store the last R-block */
	uint8_t             sdata[T1_BUFFER_SIZE];
	uint8_t             rdata[T1_BUFFER_SIZE];
} t1_state_t;

struct smartcard_private {
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
	uint32_t        Egt;          ///< 当前字节发送完毕后开始发送下一个字节须等待的时钟周期数 ///< ,以智能卡时钟周期为单位
	uint32_t        Tgt;          ///< 模块从其它状态转入发送过程时，发送第一个字节前须等待的 ///< 时钟周期数，以智能卡时钟周期为单位
	uint32_t        Wdt;          ///< 该值设定了数据接收过程中当前字节起始比特和下一字节起始比 ///< 特之间的最大等待时间，以智能卡时钟周期为单位
	uint32_t        Twdt;         ///< 该值设定了模块从其它状态转入接收状态后等待第一个字节到来 ///< 的最大等待时间，以智能卡时钟周期为单位
}GXSMART_TimeParams_t;

typedef struct s_ATR
{
	unsigned          length;
	uint8_t           TS;
	uint8_t           T0;

	struct {
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

#define SMC_VIACCESS_TYPE           0x603 /* Default values for paramenters */
#define ATR_DEFAULT_F               372
#define ATR_DEFAULT_D               1
#define ATR_DEFAULT_I               50
#define ATR_DEFAULT_N               0
#define ATR_DEFAULT_P               5
#define ATR_DEFAULT_WI              10

#define ATR_DEFAULT_BWI             4   
#define ATR_DEFAULT_CWI             13

#define ATR_DEFAULT_IFSC                32
#define ATR_DEFAULT_IFSD                32
#define ATR_DEFAULT_BGT             22
#define ATR_DEFAULT_CHK                 0       //default checksum is LRC

#define DFT_WORK_CLK                3600000//3579545     
#define ATR_PROTOCOL_TYPE_T1        1   /* Protocol type T=1 */
#define DFT_WORK_ETU                372
#define DEFAULT_BAUD_RATE           (9600 * 372)
#define DEFAULT_ETU                     (372)
#define DEFAULT_EGT                     (DEFAULT_ETU * 12)
#define DEFAULT_WDT                     (9600* DEFAULT_ETU)
#define DEFAULT_TGT                     (0)
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
#define RFU                         0
#define F_RFU                       0
#define D_RFU                       0
#define I_RFU                       0
#define ATR_ID_NUM                  16
#define ATR_FI_NUM                  16
#define ATR_DI_NUM                  16
#define ATR_I_NUM                   4

#define MASK_BIT4                   (1 << 4)
#define MASK_BIT5                   (1 << 5)
#define MASK_BIT6                   (1 << 6)
#define MASK_BIT7                   (1 << 7)

#define GET_NUMBER(TD)  ((((TD) >> 4) & 1) + (((TD) >> 5) & 1) +        \
		(((TD) >> 6) & 1) + (((TD) >> 7) & 1))

#define INVERT_BYTE(a)  ((((a) << 7) & 0x80) | \
		(((a) << 5) & 0x40) | \
		(((a) << 3) & 0x20) | \
		(((a) << 1) & 0x10) | \
		(((a) >> 1) & 0x08) | \
		(((a) >> 3) & 0x04) | \
		(((a) >> 5) & 0x02) | \
		(((a) >> 7) & 0x01))


static ATR *analyse_atr;
static const uint32_t atr_num_ib_table[16] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};
const uint32_t atr_fs_table[16] = {4000000L, 5000000L, 6000000L, 8000000L, 12000000L, 16000000L, 20000000L, 0, 0, 5000000L, 7500000L, 10000000L, 15000000L, 20000000L, 0, 0};
const uint16_t cFI[16] = { 372,372,558,744,1116,1488,1860,RFU, RFU,512,768,1024,1536,2048,RFU,RFU};
const uint16_t cDI[16] = {RFU,1,2,4,8,16,32,RFU,12,20,RFU,RFU,RFU,RFU,RFU,RFU};
static uint32_t atr_f_table[ATR_FI_NUM] ={372,372,558,744,1116,1488,1860,F_RFU,F_RFU,512,768,1024,1536,2048,F_RFU,F_RFU};
static uint32_t atr_d_table[ATR_DI_NUM] ={D_RFU,1,2,4,8,16,32,D_RFU,12,20,D_RFU,D_RFU,D_RFU,D_RFU,D_RFU,D_RFU};
static uint32_t atr_i_table[ATR_I_NUM] = {25, 50, 100, 0};
static GX_SCARD_PROTOCOL s_Protocol = GX_SCARD_PROTOCOL_T0;
uint8_t SCRes[256] = {0};

static uint8_t xor (const uint8_t * cmd, int32_t cmdlen)
{
	int32_t i;
	uint8_t checksum = 0x00;
	for (i = 0; i < cmdlen; i++)
		checksum ^= cmd[i];
	return checksum;
}

static int atr_config_parameter(struct smartcard_private *handler, ATR *atr)
{
	uint8_t i,checksum=0;
	uint8_t FI, DI, II, PI1, PI2, N, WI;
	uint32_t F, D, I, P;
	uint8_t T = 0xFF;
	uint8_t TA_AFTER_T15 = 0;
	uint8_t find_T15 = 0;

	handler->smc_clock = DFT_WORK_CLK;
	if (atr->ib[0][ATR_INTERFACE_BYTE_TA].present) {
		FI = (atr->ib[0][ATR_INTERFACE_BYTE_TA].value & 0xF0) >> 4;
		F = atr_f_table[FI];
		//robbin change
		if(F == F_RFU)
			F = ATR_DEFAULT_F;
		printf("SmartCard, ATR: Clock Rate Conversion F=%d, FI=%d\n", F, FI);
	} else {
		F = ATR_DEFAULT_F;
		printf("SmartCard, ATR: Clock Rate Conversion F=(Default)%d\n", F);
	}
	handler->F = F;

	if (atr->ib[0][ATR_INTERFACE_BYTE_TA].present) {
		DI = (atr->ib[0][ATR_INTERFACE_BYTE_TA].value & 0x0F);
		D = atr_d_table[DI];
		if(D == D_RFU)
			D = ATR_DEFAULT_D;
		printf("SmartCard, ATR: Bit Rate Adjustment Factor D=%d, DI=%d\n", D, DI);
	} else {
		D = ATR_DEFAULT_D;
		printf("SmartCard, ATR: Bit Rate Adjustment Factor D=(Default)%d\n", D);
	}
	handler->D = D;

	if (atr->ib[0][ATR_INTERFACE_BYTE_TB].present) {
		II = (atr->ib[0][ATR_INTERFACE_BYTE_TB].value & 0x60) >> 5;
		I = atr_i_table[II];
		if(I == I_RFU)
			I = ATR_DEFAULT_I;
		printf("SmartCard, ATR: Programming Current Factor I=%d, II=%d\n", I, II);
	} else {
		I= ATR_DEFAULT_I;
		printf("SmartCard, ATR: Programming Current Factor I=(Default)%d\n", I);        
	}
	handler->I = I;

	if (atr->ib[1][ATR_INTERFACE_BYTE_TB].present) {
		PI2 = atr->ib[1][ATR_INTERFACE_BYTE_TB].value;
		P = PI2;
		printf("SmartCard, ATR: Programming Voltage Factor P=%d, PI2=%d\n", P, PI2);
	} else if (atr->ib[0][ATR_INTERFACE_BYTE_TB].present) {
		PI1 = (atr->ib[0][ATR_INTERFACE_BYTE_TB].value & 0x1F);
		P = PI1;
		printf("SmartCard, ATR: Programming Voltage Factor P=%d, PI1=%d\n", P, PI1);
	} else {
		P = ATR_DEFAULT_P;
		printf("SmartCard, ATR: Programming Voltage Factor P=(Default)%d\n", P);
	}
	handler->P = P;

	if (atr->ib[0][ATR_INTERFACE_BYTE_TC].present) {
		N = atr->ib[0][ATR_INTERFACE_BYTE_TC].value;
		/*if(N == 0xFF)
		  N = 11;
		  */
	} else
		N = ATR_DEFAULT_N;
	printf("SmartCard, ATR: Extra Guardtime N=%d\n", N);
	handler->N = N;

	//for (i=0; i<ATR_MAX_PROTOCOLS; i++)
	//robbin change
	for (i=0; i<atr->pn; i++)
		if (atr->ib[i][ATR_INTERFACE_BYTE_TD].present && (0xFF == T))
		{
			/* set to the first protocol byte found */
			T = atr->ib[i][ATR_INTERFACE_BYTE_TD].value & 0x0F;
			printf("SmartCard, ATR: default protocol: T=%d\n", T);
		}
	//Try to find 1st TA after T = 15
	for (i=0; i<atr->pn; i++)
	{
		if (atr->ib[i][ATR_INTERFACE_BYTE_TD].present)
		{
			if((!find_T15)&&(0xF== (atr->ib[i][ATR_INTERFACE_BYTE_TD].value & 0x0F)))
			{
				find_T15 = 1;
				printf("SmartCard, Find T==15 at TD%d\n", i+1);
				continue;
			}
		}
		if ((find_T15)&&(atr->ib[i][ATR_INTERFACE_BYTE_TA].present))
		{
			TA_AFTER_T15 = atr->ib[i][ATR_INTERFACE_BYTE_TA].value;
			printf("SmartCard, Find 1st TA after T==15 at TA%d, value %02x\n", (i+1), TA_AFTER_T15);
			break;
		}
	}
	//if has TA2, it indicate the special protocal
	if (atr->ib[1][ATR_INTERFACE_BYTE_TA].present)
	{
		T = atr->ib[1][ATR_INTERFACE_BYTE_TA].value & 0x0F;
		printf("SmartCard, ATR: specific mode found: T=%d\n", T);
		if (atr->ib[1][ATR_INTERFACE_BYTE_TA].value & 0x10) //check TA(2), bit 5
			handler->TA2_spec = 0;        //use the default value of F/D
		else
			handler->TA2_spec = 1;         //Use the value specified in the ATR 
	}

	if (0xFF == T)
	{
		printf("SmartCard, ATR: no default protocol found in ATR. Using T=0\n");
		T = ATR_PROTOCOL_TYPE_T0;
	}
	handler->T = T;

	if(handler->D != 0)
		handler->smc_etu = handler->F/handler->D;
	handler->first_cwt = 1000;//FIRST_CWT_VAL; 
	handler->cwt = 300;//CWT_VAL;
	if(0 == T)
	{
		if (atr->ib[2][ATR_INTERFACE_BYTE_TC].present)
		{
			WI = atr->ib[2][ATR_INTERFACE_BYTE_TC].value;
		}
		else
			WI = ATR_DEFAULT_WI;
		printf("SmartCard, ATR: Work Waiting Time WI=%d\n", WI);
		handler->WI = WI;

		handler->first_cwt = handler->cwt = (960*WI*handler->F)/(handler->smc_clock/1000)+1;
	}
	else if(1 == T)
	{
		for (i = 1 ; i < atr->pn ; i++) 
		{
			/* check for the first occurance of T=1 in TDi */
			if (atr->ib[i][ATR_INTERFACE_BYTE_TD].present && 
					(atr->ib[i][ATR_INTERFACE_BYTE_TD].value & 0x0F) == ATR_PROTOCOL_TYPE_T1) 
			{
				/* check if ifsc exist */
				if (atr->ib[i + 1][ATR_INTERFACE_BYTE_TA].present)
					handler->T1.ifsc = atr->ib[i + 1][ATR_INTERFACE_BYTE_TA].value;
				else
					handler->T1.ifsc = ATR_DEFAULT_IFSC; /* default 32*/

				/* Get CWI */
				if (atr->ib[i + 1][ATR_INTERFACE_BYTE_TB].present)
					handler->CWI = atr->ib[i + 1][ATR_INTERFACE_BYTE_TB].value & 0x0F;
				else
					handler->CWI = ATR_DEFAULT_CWI; /* default 13*/
				//handler->cwt =  (((1<<(handler->CWI))+11 )*handler->smc_etu)/(handler->smc_clock/1000) + 1; 
				/*Get BWI*/
				if (atr->ib[i + 1][ATR_INTERFACE_BYTE_TB].present)
					handler->BWI = (atr->ib[i + 1][ATR_INTERFACE_BYTE_TB].value & 0xF0) >> 4;
				else
					handler->BWI = ATR_DEFAULT_BWI; /* default 4*/    
				//handler->first_cwt= (((1<<(handler->BWI))*960+11)*handler->smc_etu)/(handler->smc_clock/1000);                
				//handler->first_cwt = (11*handler->smc_etu)/(handler->smc_clock/1000)+((1<<(handler->BWI))*960*ATR_DEFAULT_F)/(handler->smc_clock/1000) + 2;   
				if (atr->ib[i + 1][ATR_INTERFACE_BYTE_TC].present)
					checksum = atr->ib[i + 1][ATR_INTERFACE_BYTE_TC].value & 0x01;
				else
					checksum = ATR_DEFAULT_CHK; /* default - LRC */
				handler->error_check_type = ((checksum==ATR_DEFAULT_CHK)?IFD_PROTOCOL_T1_CHECKSUM_LRC:IFD_PROTOCOL_T1_CHECKSUM_CRC);

			}
		}
		printf("SmartCard, T1 special: ifsc: %d,  CWI:%d,  BWI:%d, checksum:%d(3:LRC,2:CRC)\n",
				handler->T1.ifsc, handler->CWI, handler->BWI, handler->error_check_type);
	}       

	printf("SmartCard, First CWT: %d, CWT: %d\n", handler->first_cwt, handler->cwt);
	printf("SmartCard, ATR: HC:");
	for(i=0; i<atr->hbn; i++)
	{
		printf("%c ", (atr->hb)[i]);
	}
	printf("\n");


	handler->smc_etu = handler->F/handler->D;
	atr->WorkEtu = handler->smc_etu;
	printf("SmartCard, work etu : %d\n",handler->smc_etu);

	//calc the bwt,cwt,bgt for T1, in us(1/1000000 second)
	if (handler->T == 1)
	{
		/*BGT = 22 etu*/
		handler->T1.BGT =  (22*handler->smc_etu)/(handler->smc_clock/1000);
		/*CWT = (2^CWI + 11) etu*/
		handler->T1.CWT =  handler->cwt;  
		/*BWT = (2^BWI*960 + 11)etu, 
		 *Attention: BWT in ms, If calc in us, it will overflow for UINT32
		 */
		handler->T1.BWT= handler->first_cwt;

		/*Add error check type*/
		//t1_set_checksum(&handler->T1, handler->error_check_type);
		/*reset the T1 state to undead state*/
		//t1_set_param(&tp->T1, IFD_PROTOCOL_T1_STATE, SENDING);
		//t1_set_param(&(tp->T1), IFD_PROTOCOL_T1_STATE, SENDING);
		printf("SmartCard, T1 special: BGT:%d, CWT:%d, us BWT:%d  ms \n",handler->T1.BGT, handler->T1.CWT, handler->T1.BWT);
	}
	return 0;
}

static int32_t smartcard_atr_parse(uint8_t *Atr_p,uint32_t AtrLength,ATR *OpenParamsOut)
{// 0xFFFF
	uint8_t             vFi,vDi;
	int32_t             vEtu = 0;
	bool                minimumGuard = FALSE; /* flag set when N = 255 */
	bool                globalSet = FALSE; /* flag set when T = 15 detected */
	bool                TDpresent;
	uint8_t             chOffset;      /* array element index */
	uint8_t             K;      /* number of historical characters */
	uint8_t             Y;      /* indicator for the presence of I/F characters */
	uint8_t             atrByte;
	uint8_t             protocolByte;
	GX_SCARD_PROTOCOL   vProtocol= GX_SCARD_PROTOCOL_T0;
	GX_SCARD_PROTOCOL   secondaryProtocol = GX_SCARD_PROTOCOL_T0; 
	uint8_t             workWaitTime;
	uint16_t            ifsc;

	/*初始化链表TD[i]，解析ATR长度*/
	/* get and parse T0 character */
	OpenParamsOut->TimeParams.Tgt =0;
	OpenParamsOut->baud_rate = DFT_WORK_CLK;
	chOffset = 1;
	atrByte = Atr_p[chOffset];
	K = (uint8_t)(atrByte & ((uint8_t)0x0F));
	Y = (uint8_t)(atrByte & ((uint8_t)0xF0));

	if(Y != 0)
	{
		//TA1, TB1, TC1 & TD1 characters
		if (Y & ((uint8_t)TA_BIT))  /* TA1 character present */
		{
			chOffset++;
			atrByte = Atr_p[chOffset];
			//printf("\n[Smart Card]---->>TA1:0x%02x",atrByte);
			vFi = (uint8_t)(((uint8_t)(atrByte & ((uint8_t)0xF0))) >> ((uint8_t)4));
			vDi = (uint8_t)(atrByte & ((uint8_t)0x0F));
			if( cFI[vFi] != RFU && cDI[vDi] != RFU)
			{
				vEtu = cFI[vFi] / cDI[vDi];
				OpenParamsOut->Etu = vEtu;
			}
			else
			{
				printf("\nSmartCard, etu error-------->>Et\n");
				OpenParamsOut->Etu = vEtu =DFT_WORK_ETU;
				//return 1;
			}
			//printf("\n[Smart Card]------------>>Etu:%d",vEtu);
		}
		else
		{
			printf("\nSmartCard, ---->>NO TA1");
			OpenParamsOut->Etu = vEtu = DFT_WORK_ETU;
		}

		if (!memcmp(Atr_p+4, "IRDETO", 6))
		{
			OpenParamsOut->Etu = vEtu = 625;
			OpenParamsOut->baud_rate = 6000000;
			//return 1;
		}
		else if (!((Atr_p[1]!=0x77) || ((Atr_p[2]!=0x18) && (Atr_p[2]!=0x11) && (Atr_p[2]!=0x19)) || (Atr_p[9]!=0x68))) 
		{
			if(vEtu < DFT_WORK_ETU)
			{
				OpenParamsOut->Etu = vEtu = DFT_WORK_ETU;
				OpenParamsOut->baud_rate = DEFAULT_BAUD_RATE;
			}
			OpenParamsOut->card_type= SMC_VIACCESS_TYPE;
		}
		else if (!((Atr_p[6]!=0xC4) || (Atr_p[9]!=0x8F) || (Atr_p[10]!=0xF1)))
		{
			if(vEtu < DFT_WORK_ETU)
			{
				OpenParamsOut->Etu = vEtu = DFT_WORK_ETU;
				OpenParamsOut->baud_rate = DEFAULT_BAUD_RATE;
			}
		}

		if (Y & ((uint8_t)TB_BIT))  /* TB1 character present */
		{
			chOffset++;
			atrByte = Atr_p[chOffset];   /* programming voltage is not controlled by emma */
			/* allow values for 5Vpp this assumes the hardware
			   has Vcc connected to Vpp */
			//printf("\n[Smart Card]---->>TB1:0x%02x",atrByte);
			switch(atrByte)
			{
				case 0x00:   /* Vpp not connected */
				case 0x80:   /* bits 7:1 not coded */
				case 0x05:   /* 5V 25mA max */
				case 0x25:   /* 5V 50mA max */
					/* acceptable values */
					break;
				default:
					/* any other values for Vpp fail - if reuired this may have to be a
					   hardware specific build option */
					break;
			}
		}
		else
		{
			printf("\nSmartCard, ---->>NO TB1");
		}
		if (Y & ((uint8_t)TC_BIT))  /* TC1 character present */
		{
			chOffset++;
			printf("\nSmartCard, ---->>TC1:0x%02x",Atr_p[chOffset]);
			if(Atr_p[chOffset] == 255)
			{
				/* special meaning (EMV 4.3.3.3) (ISO 6.5.3) */
				minimumGuard = TRUE; /* if T = 1 -> 11etu guard */
				OpenParamsOut->TimeParams.Egt =  11*vEtu;
			}
			else
			{
				//atrParams->guardTime = Atr_p[chOffset];
				OpenParamsOut->TimeParams.Egt = (Atr_p[chOffset]) * vEtu;
				if((Atr_p[chOffset]) ==0)
				{
					OpenParamsOut->TimeParams.Egt =  20*vEtu;
				}
			}
			//printf("\n[Smart Card]---->>TimeParams.Egt:%d Guard:%d",OpenParamsOut->TimeParams.Egt,minimumGuard);
		}
		else
		{
			printf("\nSmartCard, ---->>NO TC1");
			OpenParamsOut->TimeParams.Egt = 20*OpenParamsOut->Etu;
		}
		if (Y & ((uint8_t)TD_BIT))  /* TD1 character present */
		{
			chOffset++;
			protocolByte = (uint8_t)(Atr_p[chOffset] & ((uint8_t)0x0F));
			//printf("\n[Smart Card]---->>TD1:0x%02x",Atr_p[chOffset]);
			switch (protocolByte)
			{
				case 0x00:
					vProtocol = GX_SCARD_PROTOCOL_T0;
					break;
				case 0x01:
					vProtocol = GX_SCARD_PROTOCOL_T1;
					//if(minimumGuard)
					//cardType = GX_SCARD_TYPE_11ETU;
					break;
				default:
					vProtocol = GX_SCARD_PROTOCOL_INVALID;
			}
			//printf("\n[Smart Card]---->>Protocol:%d",vProtocol);
			Y = (uint8_t)(Atr_p[chOffset] & ((uint8_t)0xF0));
		}
		else
		{
			/* TD1 character is not present - default to T=0*/
			printf("\nSmartCard, ---->>NO TD1");
			vProtocol = GX_SCARD_PROTOCOL_T0;
			Y = 0;  /* no more I/F characters */
		}

		OpenParamsOut->TimeParams.Wdt  = 45 * vEtu;
		OpenParamsOut->TimeParams.Twdt = 2 * 9600 * vEtu;
		if (Y != 0)
		{
			// TA2, TB2, TC2 & TD2 characters
			if (Y & ((uint8_t)TA_BIT)) /* TA2 character present */
			{
				chOffset++; /* EMV 4.3.3.5 , ISO 7816-3 6.6 */
				protocolByte = (uint8_t)(Atr_p[chOffset] & ((uint8_t)0x0F));
				//printf("\n[Smart Card]---->>TA2:0x%02x",Atr_p[chOffset]);
				switch (protocolByte)
				{
					case 0x00:
						vProtocol = GX_SCARD_PROTOCOL_T0;
						break;
					case 0x01:
						vProtocol = GX_SCARD_PROTOCOL_T1;
						break;
					default:
						vProtocol = GX_SCARD_PROTOCOL_INVALID;
				}

				if(Atr_p[chOffset] & ((uint8_t)0x10)) /* bit5 set? */
				{
					/* for ISO - F and D parameters are implicit selection
					   PPS not issued so the first offered protocol shall apply
					   using Fd and Dd */
					vFi = 1;
					vDi = 1;
					vEtu = cFI[vFi] / cDI[vDi];
					OpenParamsOut->Etu = vEtu;
					if (!memcmp(Atr_p+4, "IRDETO", 6))
					{
						OpenParamsOut->Etu = vEtu = 625;
					}
					printf("\nSmartCard, ---->>Don't Change the ETU");
				}
			}
			else
			{
				printf("\nSmartCard, ---->>NO TA2");
			}
			if (Y & ((uint8_t)TB_BIT)) /* TB2 character present */
			{
				/* programming voltage not supported */
				chOffset++;
				printf("\nSmartCard, ---->>TB2:0x%02x",Atr_p[chOffset]);
			}
			else
			{
				printf("\nSmartCard, ---->>NO TB2");
			}
			if (Y & ((uint8_t)TC_BIT))  /* TC2 character present */
			{
				chOffset++;
				printf("\nSmartCard, ---->>TC2:0x%02x",Atr_p[chOffset]);
				workWaitTime = Atr_p[chOffset];//Just For T0 Protocol
			}
			else
			{
				printf("\nSmartCard, ---->>NO TC2");
			}
			if (Y & ((uint8_t)TD_BIT))  /* TD2 character present */
			{
				chOffset++;
				printf("\nSmartCard, ---->>TD2:0x%02x",Atr_p[chOffset]);
				protocolByte = (uint8_t)(Atr_p[chOffset] & ((uint8_t)0x0F));
				switch (protocolByte)
				{
					case 0x00:
						secondaryProtocol = GX_SCARD_PROTOCOL_T0;
						break;
					case 0x01:
						secondaryProtocol = GX_SCARD_PROTOCOL_T1;
						break;
					case 0x0f:
						globalSet = TRUE;
						printf("\nSmartCard, ---->>globalSet:%d",globalSet);
						/*  the following interface bytes are global */
						break;
					default:
						secondaryProtocol = GX_SCARD_PROTOCOL_INVALID;
				}
				Y = (uint8_t)(Atr_p[chOffset] & ((uint8_t)0xF0));
			}
			else
			{
				/* TD2 character is not present */
				Y = 0;  /* no more I/F characters */
				printf("\nSmartCard, ---->>NO TD2");
			}

			if (Y != 0)
			{
				// TA3, TB3, TC3 & TD3 characters
				if ((Y & ((uint8_t)TA_BIT))) /* TA3 character present -  EMV 4.3.3.9 - ISO 9.5 */
				{
					chOffset++;
					printf("\nSmartCard, ---->>TA3:0x%02x",Atr_p[chOffset]);
					if(globalSet)
					{
						/* T==15 for TD(2) so this byte is global - not T=1 specific */
						/* clock stop and class not supported by this hardware so ignore */
					}
					else if((Atr_p[chOffset] > 0x0f)&&(Atr_p[chOffset] != 0xff))
					{
						ifsc = Atr_p[chOffset];
						printf("\nSmartCard, ---->>ifsc:%d",ifsc);
					}
					else
					{   /* reject values 0-0f and 0xff */

					}
				}
				else
				{
					printf("\nSmartCard, ---->>NO TA3");
				}
				if (Y & ((uint8_t)TB_BIT)) /* TB3 character present */
				{               /* EMV 4.3.3.10 - ISO 9.5.3*/
					chOffset++;
					printf("\nSmartCard, ---->>TB3:0x%02x",Atr_p[chOffset]);
					if(globalSet)
					{
						/* T==15 for TD(2) so this byte is global - not T=1 specific */
						/* Ignore the parameters for now */
					}
					else    /* T = 1 specific */
					{
						atrByte = Atr_p[chOffset];
						//atrParams->blockWaitTime = (uint8_t)(((uint8_t)(atrByte & ((uint8_t)0xF0))) >> ((uint8_t)4));  /* T=1 */
						//atrParams->characterWaitTime = (uint8_t)(atrByte & ((uint8_t)0x0F));         /* T=1 */
						/* ISO reject max values */
						OpenParamsOut->TimeParams.Wdt  = ((1 << (atrByte & 0x0F))+11) * vEtu;
						OpenParamsOut->TimeParams.Twdt = ((1 << (atrByte >> 4))+11) * 960 * vEtu;
						printf("\nSmartCard, ---->>Wdt:%d Twdt:%d",((1 << (atrByte & 0x0F))+11),((1 << (atrByte >> 4))+11)* 960);
					}
				}
				else
				{
					printf("\nSmartCard, ---->>NO TB3");
				}
				if (Y & ((uint8_t)TC_BIT)) /* TC3 character present */
				{               /* EMV 4.3.3.11  - ISO 9.5.4 */
					chOffset++;
					printf("\nSmartCard, ---->>TC3:0x%02x",Atr_p[chOffset]);
					if(globalSet)
					{
						/* T==15 for TD(2) so this byte is global - not T=1 specific */
						/* Ignore the parameters for now */
					}
					else   /* T = 1 error detect check */
					{
						if(Atr_p[chOffset] != 0)
						{
							/* for EMV reject any other value than zero */
							//result = MMAC_SCARD_FAIL;
						}
						else if((Atr_p[chOffset] & 0x1) == 0x1)
						{
							/* for ISO CRC mode is acceptable */
							//atrParams->EDC = MMAC_SCARD_CRC_EDC;
							printf("\nSmartCard, ---->>CRC Check");
						}
						printf("\nSmartCard, ---->>LRC Check");
						/* default is LRC mode */
					}
				}
				else
				{
					printf("\nSmartCard, ---->>NO TC3");
				}
				if (Y & ((uint8_t)TD_BIT))  /* TD3 character present */
				{
					chOffset++;
					printf("\nSmartCard, ---->>TD3:0x%02x",Atr_p[chOffset]);
					{
						/* for ISO there may be multiple interface bytes defined for the same protocol */
						/* (do not use this data for now - just increment i as required) */
						do {
							Y = (uint8_t)(Atr_p[chOffset] & ((uint8_t)0xF0));
							if(Y & ((uint8_t)TD_BIT))
								TDpresent = TRUE;
							else 
								TDpresent = FALSE;
							Y = (Y >> 4);
							/* while Tx(i) chars present */
							while(Y != 0)
							{
								chOffset++;
								Y = (Y >> 0x01);
							}
						}while(TDpresent);
					}
				}
				else
				{
					printf("\nSmartCard, ---->>NO TD3");
				}
			}
		}

	}

	printf("\nSmartCard, ---->>Smartcard Protocol:%d\n",vProtocol);

	chOffset += K;
	//check for TCK character
	if ((vProtocol == GX_SCARD_PROTOCOL_T1)||
			(secondaryProtocol == GX_SCARD_PROTOCOL_T1))
	{
		/* TCK character present */
		chOffset++;
		printf("\nSmartCard, ---->>Smartcard have TCK:0x%02x!~~\n",Atr_p[chOffset]);
		atrByte = Atr_p[chOffset];       /* TCK character */
		chOffset++;
		if (chOffset != AtrLength)
		{
			return 1;
		}
		else
		{
			/* perform XOR check on ATR */
			atrByte = Atr_p[1];
			for (chOffset = 2; chOffset < AtrLength; ++chOffset)
			{
				atrByte ^= Atr_p[chOffset];
			}
			if (atrByte != 0)
			{
				/* check failed */
				return 1;
			}
		}
	}

	s_Protocol = vProtocol;
	return 0;
}

static int32_t ATR_InitFromArray (ATR * atr, uint8_t atr_buffer[256], uint32_t length)
{
	uint8_t TDi;
	uint8_t buffer[256];
	uint32_t pointer = 0, pn = 0;

	/* Check size of buffer */
	//在一次测试中，length 等于382 ，此时会死机
	if (length < 2 || length > 256)
		return (ATR_MALFORMED);

	smartcard_atr_parse(atr_buffer,length,atr);
	/* Check if ATR is from a inverse convention card */
	if (atr_buffer[0] == 0x03)
	{
		for (pointer = 0; pointer < length; pointer++)
			buffer[pointer] = ~(INVERT_BYTE (atr_buffer[pointer]));
	}
	else
	{
		memcpy (buffer, atr_buffer, length);
	}

	if(atr_buffer[0] != 0x3b && atr_buffer[0] != 0x3f)
	{
		return ATR_NOT_FOUND;
	}

	/* Store T0 and TS */
	atr->TS = buffer[0];

	atr->T0 = TDi = buffer[1];
	pointer = 1;

	/* Store number of historical bytes */
	atr->hbn = TDi & 0x0F;

	/* TCK is not present by default */
	(atr->TCK).present = FALSE;

	/* Extract interface bytes */
	while (pointer < length)
	{
		/* Check buffer is long enought */
		if (pointer + atr_num_ib_table[(0xF0 & TDi) >> 4] >= length)
		{
			return (ATR_MALFORMED);
		}

		/* Check TAi is present */
		if ((TDi | 0xEF) == 0xFF)
		{
			pointer++;
			atr->ib[pn][ATR_INTERFACE_BYTE_TA].value = buffer[pointer];
			atr->ib[pn][ATR_INTERFACE_BYTE_TA].present = TRUE;
		}
		else
		{
			atr->ib[pn][ATR_INTERFACE_BYTE_TA].present = FALSE;
		}

		/* Check TBi is present */
		if ((TDi | 0xDF) == 0xFF)
		{
			pointer++;
			atr->ib[pn][ATR_INTERFACE_BYTE_TB].value = buffer[pointer];
			atr->ib[pn][ATR_INTERFACE_BYTE_TB].present = TRUE;
		}
		else
		{
			atr->ib[pn][ATR_INTERFACE_BYTE_TB].present = FALSE;
		}

		/* Check TCi is present */
		if ((TDi | 0xBF) == 0xFF)
		{
			pointer++;
			atr->ib[pn][ATR_INTERFACE_BYTE_TC].value = buffer[pointer];
			atr->ib[pn][ATR_INTERFACE_BYTE_TC].present = TRUE;
		}
		else
		{
			atr->ib[pn][ATR_INTERFACE_BYTE_TC].present = FALSE;
		}

		/* Read TDi if present */
		if ((TDi | 0x7F) == 0xFF)
		{
			pointer++;
			TDi = atr->ib[pn][ATR_INTERFACE_BYTE_TD].value = buffer[pointer];
			atr->ib[pn][ATR_INTERFACE_BYTE_TD].present = TRUE;
			(atr->TCK).present = ((TDi & 0x0F) != ATR_PROTOCOL_TYPE_T0);
			if (pn >= ATR_MAX_PROTOCOLS)
				return (ATR_MALFORMED);
			pn++;
		}
		else
		{
			atr->ib[pn][ATR_INTERFACE_BYTE_TD].present = FALSE;
			break;
		}
	}

	/* Store number of protocols */
	atr->pn = pn + 1;

	/* Store historical bytes */
	if (pointer + atr->hbn >= length) {
		printf("\nSmartCard, ATR is malformed, it reports %i historical bytes but there are only %i\n",atr->hbn, length-pointer-2);
		if (length-pointer >= 2)
			atr->hbn = length-pointer-2;
		else {
			atr->hbn = 0;
			atr->length = pointer + 1;
			return (ATR_MALFORMED);
		}

	}

	memcpy (atr->hb, buffer + pointer + 1, atr->hbn);
	pointer += (atr->hbn);

	/* Store TCK  */
	if ((atr->TCK).present)
	{       
		if (pointer + 1 >= length)
			return (ATR_MALFORMED);

		pointer++;

		(atr->TCK).value = buffer[pointer];
	}

	atr->length = pointer + 1;

	// check that TA1, if pn==1 , has a valid value for FI
	if ( (atr->pn==1) && (atr->ib[pn][ATR_INTERFACE_BYTE_TA].present == TRUE)) {
		uint8_t FI;
		//printf("\nTA1 = %02x\n",atr->ib[pn][ATR_INTERFACE_BYTE_TA].value);
		FI=(atr->ib[pn][ATR_INTERFACE_BYTE_TA].value & 0xF0)>>4;
		//printf("\nFI = %02x\n",FI);
		if(atr_fs_table[FI]==0) {
			printf("\nSmartCard, Invalid ATR as FI is not returning a valid frequency value\n");
			return (ATR_MALFORMED);
		}
	}

	// check that TB1 < 0x80
	if ( (atr->pn==1) && (atr->ib[pn][ATR_INTERFACE_BYTE_TB].present == TRUE)) {
		if(atr->ib[pn][ATR_INTERFACE_BYTE_TB].value > 0x80) {
			printf("\nSmartCard, Invalid ATR as TB1 has an invalid value\n");
			return (ATR_MALFORMED);
		}
	}



	{
		struct smartcard_private handler = {0};
		atr_config_parameter(&handler,  atr); 

		if(handler.T == 1)
		{
			if(handler.D != 0)
			{
				handler.smc_etu  = handler.F/handler.D;
				printf("SmartCard, ---->> work etu : %d\n",handler.smc_etu );
			}
			/*BGT = 22 etu*/
			handler.T1.BGT =  (22*handler.smc_etu)/(handler.smc_clock/1000);
			/*CWT = (2^CWI + 11) etu*/
			handler.T1.CWT =  handler.cwt;  
			/*BWT = (2^BWI*960 + 11)etu, 
			 *Attention: BWT in ms, If calc in us, it will overflow for UINT32
			 */
			handler.T1.BWT= handler.first_cwt;
			printf("SmartCard, ---->> T1 special: BGT:%d, CWT:%d, us BWT:%d  ms \n",handler.T1.BGT, handler.T1.CWT, handler.T1.BWT);

		}
		else if(handler.T == 14)
		{

		}
		else
		{
			if(handler.D != 0)
			{
				handler.smc_etu  = handler.F/handler.D;
				printf("SmartCard, work etu : %d\n",handler.smc_etu);
			}   
		}

	}

	return (ATR_OK);
}

static int32_t ATR_GetHistoricalBytes (ATR *atr, uint8_t hist[15], uint32_t *length)
{
	if (atr->hbn == 0)
		return (ATR_NOT_FOUND);

	(*length) = atr->hbn;
	memcpy (hist, atr->hb, atr->hbn);
	return (ATR_OK);
}

static void smartcard_atr_analyse(uint8_t *atr)
{
	if (!memcmp(atr+4, "IRDETO", 6)) { //[None,Irdeto,Conax,Viaccess,Out]
		//reader->acs57=0;
		printf("\nSmartCard, detect irdeto card\n");
	}

	if (!((atr[6]!=0xC4) || (atr[9]!=0x8F) || (atr[10]!=0xF1))) {
		printf("\nSmartCard, [cryptoworks-reader] card detected\n");
		printf("\nSmartCard, [cryptoworks-reader] type: CryptoWorks\n");
	}

	if (!((atr[1]!=0x77) || ((atr[2]!=0x18) && (atr[2]!=0x11) && (atr[2]!=0x19)) || (atr[9]!=0x68))) {
		printf("\nSmartCard, detect Viaccess card\n");
	}

	//Dre Card
	if (!((atr[0] != 0x3b) || (atr[1] != 0x15) || (atr[2] != 0x11) || (atr[3] != 0x12 || atr[4] != 0xca || atr[5] != 0x07))) {
		uint8_t checksum = xor (atr + 1, 6);
		printf("\nSmartCard, ====detect Dre card===>>\n");

		if (checksum != atr[7])
			printf ("SmartCard, [dre-reader] warning: expected ATR checksum %02x, smartcard reports %02x", checksum, atr[7]);

		switch (atr[6]) {
			case 0x11:
				printf("\nSmartCard, detect Tricolor Centr card\n");
				break;                  //59 type card = MSP (74 type = ATMEL)
			case 0x12:
				printf("\nSmartCard, detect Cable TV card\n");
				break;
			case 0x14:
				printf("\nSmartCard, detect Tricolor Syberia / Platforma HD new card\n");
				break;                  //59 type card
			case 0x15:
				printf("\nSmartCard, detect Platforma HD / DW old card\n");
				break;                  //59 type card
			default:
				printf("\nSmartCard, detect Unknown card\n");
				break;
		}
	}

	if(memcmp(atr+11,"DNASP240",8)==0 || memcmp(atr+11,"DNASP241", 8)==0) {
		printf("\nSmartCard, detect nagra 3 NA card\n");
	} else if (memcmp(atr+11, "DNASP", 5)==0) {
		printf("\nSmartCard, detect native nagra card\n");
	} else if (memcmp(atr+11, "TIGER", 5)==0 || (memcmp(atr+11, "NCMED", 5)==0)) {
		printf("\nSmartCard, detect nagra tiger card\n");
	} else if ((!memcmp(atr+4, "IRDETO", 6)) && ((atr[14]==0x03) && (atr[15]==0x84) && (atr[16]==0x55))) {
		printf("\nSmartCard, detect irdeto tunneled nagra card\n");
	} else {
		unsigned char hist[64];
		uint32_t hist_size = 0;
		ATR_GetHistoricalBytes(analyse_atr, hist, &hist_size);
		if (!((hist_size < 4) || (memcmp(hist,"0B00",4)))) {
			printf("\nSmartCard, detect Conax card\n");
		} else if(!((hist_size < 7) || (hist[1] != 0xB0) || (hist[4] != 0xFF) || (hist[5] != 0x4A) || (hist[6] != 0x50))) {
			printf("\nSmartCard, [videoguard12/videoguard2]detect NDS card\n");
		} else if(!((hist_size < 7) || (hist[1] != 0xB0) || (hist[3] != 0x4A) || (hist[4] != 0x50))) {
			printf("\nSmartCard, [videoguard1]detect NDS card\n");
		}
	}

}

void parse_atr(uint8_t *atr,uint32_t len)
{
	printf("SmartCard, Start analyse the ATR!!\n");
	analyse_atr = malloc(sizeof(ATR));
	if(NULL == analyse_atr)
		printf("\nSmartCard, malloc failed\n");
	if( (ATR_InitFromArray(analyse_atr, atr, len) != ATR_OK)) {
		free(analyse_atr);
		analyse_atr = NULL;
	}
	smartcard_atr_analyse(atr);

	GxSmcTimeParams ConfigP = {0};
	analyse_atr->WorkEtu = (analyse_atr->WorkEtu > 0)?(analyse_atr->WorkEtu):(analyse_atr->Etu);
	ConfigP.baud_rate = analyse_atr->baud_rate;//3571200;
	ConfigP.egt = (analyse_atr->TimeParams.Egt*analyse_atr->Etu)/analyse_atr->WorkEtu;
	ConfigP.tgt = (analyse_atr->TimeParams.Tgt*analyse_atr->Etu)/analyse_atr->WorkEtu;
	ConfigP.twdt = (analyse_atr->TimeParams.Twdt*analyse_atr->Etu)/analyse_atr->WorkEtu;
	ConfigP.wdt = (analyse_atr->TimeParams.Wdt*analyse_atr->Etu)/analyse_atr->WorkEtu;
	ConfigP.etu = analyse_atr->Etu;
	printf("\nSMART CARD, bud = %d, work etu = %d\n", analyse_atr->baud_rate, analyse_atr->WorkEtu);
	printf("\nbud = %d, egt = %d,tgt = %d,twdt = %d,wdt = %d,etu = %d\n",ConfigP.baud_rate,ConfigP.egt,ConfigP.tgt,ConfigP.twdt,ConfigP.wdt,ConfigP.etu);

	GxCas_Smc_Config(&ConfigP);
	printf("\nSmartCard, successfully!!!\n");

	free(analyse_atr);
	analyse_atr = NULL;
	return;
}

