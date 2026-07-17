#include "gxcore.h"
#include "gxcas_dbg.h"
#include "gxcas_smartcard.h"

#define DEFAULT_ETU                 (372)
#define DEFAULT_BAUDRATE            (9600 * DEFAULT_ETU)
#define DEFAULT_EGT                 (12   * DEFAULT_ETU)
#define DEFAULT_WDT                 (9600 * DEFAULT_ETU)
#define DEFAULT_TGT                 (0)
#define DEFAULT_TWDT                (0x8FFFFF)

/* T14协议参数 */
#define T14_RUNTIME_ETU                     620
#define T14_RUNTIME_BAUD_RATE               (9600*T14_RUNTIME_ETU)

#define T14_RUNTIME_EGT                     (T14_RUNTIME_ETU *12)
#define T14_RUNTIME_WDT                     (45 * T14_RUNTIME_ETU)
#define T14_RUNTIME_TGT                     (2* T14_RUNTIME_ETU)
#define T14_RUNTIME_TWDT                    (0x8fffff)


/* defines for ATR bit flags */
#define TAMASK                      (0x10)//Y1 bit 5
#define TBMASK                      (0x20)//Y1 bit 6
#define TCMASK                      (0x40)//Y1 bit 7
#define TDMASK                      (0x80)//Y1 bit 8

#define RFU                         0
#define FI_NUM                      16
#define DI_NUM                      16
#define I_NUM                       4

#define INVERT_BYTE(a)  ((((a) << 7) & 0x80) | \
		(((a) << 5) & 0x40) | \
		(((a) << 3) & 0x20) | \
		(((a) << 1) & 0x10) | \
		(((a) >> 1) & 0x08) | \
		(((a) >> 3) & 0x04) | \
		(((a) >> 5) & 0x02) | \
		(((a) >> 7) & 0x01))


static const uint16_t cFI[FI_NUM] = { 372,372,558,744,1116,1488,1860,RFU, RFU,512,768,1024,1536,2048,RFU,RFU};
static const uint16_t cDI[DI_NUM] = {RFU,1,2,4,8,16,32,RFU,12,20,RFU,RFU,RFU,RFU,RFU,RFU};
static const uint32_t cII[I_NUM] = {25, 50, 100, 0};

typedef struct sATR
{
	uint32_t          Etu;
	uint32_t          baud_rate;
	uint32_t          Egt;          ///< 当前字节发送完毕后开始发送下一个字节须等待的时钟周期数 ///< ,以智能卡时钟周期为单位
	uint32_t          Tgt;          ///< 模块从其它状态转入发送过程时，发送第一个字节前须等待的 ///< 时钟周期数，以智能卡时钟周期为单位
	uint32_t          Wdt;          ///< 该值设定了数据接收过程中当前字节起始比特和下一字节起始比 ///< 特之间的最大等待时间，以智能卡时钟周期为单位
	uint32_t          Twdt;         ///< 该值设定了模块从其它状态转入接收状态后等待第一个字节到来 ///< 的最大等待时间，以智能卡时钟周期为单位
	GxSmcProtocol protocol;
}ATR;

static int32_t atr_parse (ATR *atr, uint8_t *in, uint32_t length)
{
	uint32_t i;
	uint8_t rawatr[256];

	if (length < 2 || length > 256){
		CAS_DBG(SMC,"atr len %d error!\n",length);
		return -1;
	}

	/* Check if ATR is from a inverse convention card */
	if (in[0] == 0x03) {
		for (i = 0; i < length; i++)
			rawatr[i] = ~(INVERT_BYTE (in[i]));
	} else {
		memcpy (rawatr, in, length);
	}

	CAS_DBG(SMC,"ATR %d [ ",length);
	for(i=0;i<length;i++)
		CAS_DBG(SMC,"0x%x ",rawatr[i]);

	CAS_DBG(SMC,"]\n");

	uint8_t             vFi,vDi,vIi;
	int32_t             vEtu = DEFAULT_ETU;
	bool                globalSet = FALSE; /* flag set when T = 15 detected */
	bool                TDpresent;
	uint8_t             offset;      /* array element index */
	uint8_t             K;      /* number of historical characters */
	uint8_t             Y;      /* indicator for the presence of I/F characters */
	uint8_t             TxByte;
	uint8_t             protocol;
	GxSmcProtocol   vProtocol= GXSMC_PROTCL_T0;
	GxSmcProtocol   secondaryProtocol = GXSMC_PROTCL_T0; uint16_t            ifsc;

	atr->baud_rate = DEFAULT_BAUDRATE;
	atr->Etu = vEtu;
	atr->Tgt =0;
	atr->Egt = 20*vEtu;

	offset = 0;
	TxByte = rawatr[offset];//TS
	if(TxByte != 0x3b && TxByte != 0x3f)
		return -1;

	offset++;
	TxByte = rawatr[offset];//GXSMC_PROTCL_T0
	Y = (TxByte & 0xF0);
	K = (TxByte & 0x0F);

	if(Y != 0) { //TA1, TB1, TC1 & TD1 characters
		if (Y & TAMASK) { /* TA1 character present */
			offset++;
			TxByte = rawatr[offset];
			CAS_DBG(SMC,"ATR---->>TA1:0x%02x\n",TxByte);
			vFi = ((TxByte & 0xF0) >> 4);
			vDi = ((TxByte & 0x0F) >> 0);
			if( cFI[vFi] != RFU && cDI[vDi] != RFU) {
				vEtu = cFI[vFi] / cDI[vDi];
				atr->Etu = vEtu;
				CAS_DBG(SMC,"ATR: Clock Rate Conversion F=%d, FI=%d\n", cFI[vFi], vFi);
				CAS_DBG(SMC,"ATR: Bit Rate Adjustment Factor D=%d, DI=%d\n", cDI[vDi], vDi);
			}
		}

		if (Y & TBMASK) { /* TB1 character present */
			offset++;
			TxByte = rawatr[offset];   /* programming voltage is not controlled by emma */
			/* allow values for 5Vpp this assumes the hardware has Vcc connected to Vpp */
			CAS_DBG(SMC,"ATR---->>TB1:0x%02x\n",TxByte);
			vIi = (TxByte & 0x60) >> 5;
			CAS_DBG(SMC,"ATR: Programming Current Factor I=%d, II=%d\n", cII[vIi], vIi);
			CAS_DBG(SMC,"ATR: Programming Voltage Factor P=%d, PI1=%d\n", (TxByte & 0x1F), (TxByte & 0x1F));
			switch(TxByte) {
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

		if (Y & TCMASK) { /* TC1 character present */
			offset++;
			TxByte = rawatr[offset];
			CAS_DBG(SMC,"ATR---->>TC1:0x%02x\n",TxByte);
			CAS_DBG(SMC,"ATR: Extra Guardtime N=%d\n", TxByte);
			if(TxByte == 255) { /* special meaning (EMV 4.3.3.3) (ISO 6.5.3) */
				atr->Egt =  11*vEtu;//GXSMC_PROTCL_T1 11*etu t0 10*etu 最短时间间隔
			} else if(TxByte ==0) {
				atr->Egt =  20*vEtu;
			}else{
				atr->Egt = TxByte * vEtu;
			}
		}

		if (Y & TDMASK) { /* TD1 character present */
			offset++;
			TxByte = rawatr[offset];
			CAS_DBG(SMC,"ATR---->>TD1:0x%02x\n",TxByte);
			protocol = TxByte & 0x0F;
			switch (protocol) {
				case 0x00:
					atr->baud_rate = 3600000;
					vProtocol = GXSMC_PROTCL_T0;
					break;
				case 0x01:
					vProtocol = GXSMC_PROTCL_T1;
					break;
				case 0xe:
					vProtocol = GXSMC_PROTCL_T14;
					break;
				default:
					vProtocol = GXSMC_PROTCL_INVALID;
			}
			Y = TxByte & 0xF0;
			CAS_DBG(SMC,"ATR: protocol: T=%d\n", protocol);
		} else { /* TD1 character is not present - default to T=0*/
			vProtocol = GXSMC_PROTCL_T0;
			CAS_DBG(SMC,"ATR: default protocol: T=0\n");
			Y = 0;  /* no more I/F characters */
		}

		atr->Wdt  = 45 * vEtu;
		atr->Twdt = 2 * 9600 * vEtu;
		if (Y != 0) { // TA2, TB2, TC2 & TD2 characters
			if (Y & TAMASK) {
				offset++; /* EMV 4.3.3.5 , ISO 7816-3 6.6 */
				TxByte = rawatr[offset];
				CAS_DBG(SMC,"ATR---->>TA2:0x%02x\n",TxByte);
				protocol = TxByte & 0x0F;
				switch (protocol) {
					case 0x00:
						vProtocol = GXSMC_PROTCL_T0;
						break;
					case 0x01:
						vProtocol = GXSMC_PROTCL_T1;
						break;
					case 0xe:
						vProtocol = GXSMC_PROTCL_T14;
						break;
					default:
						vProtocol = GXSMC_PROTCL_INVALID;
				}

				if(TxByte & 0x10) {/* bit5 set? */
					/* for ISO - F and D parameters are implicit selection
					   PPS not issued so the first offered protocol shall apply
					   using Fd and Dd */
					vFi = 1;
					vDi = 1;
					vEtu = cFI[vFi] / cDI[vDi];
					atr->Etu = vEtu;
					CAS_DBG(SMC,"ATR---->>Don't Change the ETU\n");
				}
			}

			if (Y & (TBMASK)) { /* programming voltage not supported */
				offset++;
				TxByte = rawatr[offset];
				CAS_DBG(SMC,"ATR---->>TB2:0x%02x\n",TxByte);
			}

			if (Y & (TCMASK)) {
				offset++;
				TxByte = rawatr[offset];
				CAS_DBG(SMC,"ATR---->>TC2:0x%02x\n",TxByte);
			}

			if (Y & (TDMASK)) {
				offset++;
				TxByte = rawatr[offset];
				CAS_DBG(SMC,"ATR---->>TD2:0x%02x\n",TxByte);
				protocol = (TxByte & 0x0F);
				switch (protocol)
				{
					case 0x00:
						secondaryProtocol = GXSMC_PROTCL_T0;
						break;
					case 0x01:
						secondaryProtocol = GXSMC_PROTCL_T1;
						break;
					case 0xe:
						secondaryProtocol= GXSMC_PROTCL_T14;
						break;
					case 0x0f:
						globalSet = TRUE;
						CAS_DBG(SMC,"ATR---->>globalSet:%d\n",globalSet);
						/*  the following interface bytes are global */
						break;
					default:
						secondaryProtocol = GXSMC_PROTCL_INVALID;
				}
				Y = (TxByte & 0xF0);
			} else {
				Y = 0;  /* no more I/F characters */
			}

			if (Y != 0) { // TA3, TB3, TC3 & TD3 characters
				if ((Y & (TAMASK))) { /* TA3 character present -  EMV 4.3.3.9 - ISO 9.5 */
					offset++;
					TxByte = rawatr[offset];
					CAS_DBG(SMC,"ATR---->>TA2:0x%02x\n",TxByte);
					if(globalSet) {
						/* T==15 for TD(2) so this byte is global - not T=1 specific */
						/* clock stop and class not supported by this hardware so ignore */
					} else if((rawatr[offset] > 0x0f)&&(rawatr[offset] != 0xff)) {
						ifsc = rawatr[offset];
						CAS_DBG(SMC,"ATR---->>ifsc:%d\n",ifsc);
					} else {   /* reject values 0-0f and 0xff */
					}
				}

				if (Y & (TBMASK)) { /* TB3 character present */ /* EMV 4.3.3.10 - ISO 9.5.3*/
					offset++;
					TxByte = rawatr[offset];
					CAS_DBG(SMC,"ATR---->>TB2:0x%02x\n",TxByte);
					if(globalSet) {
						/* T==15 for TD(2) so this byte is global - not T=1 specific */
						/* Ignore the parameters for now */
					} else {    /* T = 1 specific */
						TxByte = rawatr[offset];
						/* ISO reject max values */
						atr->Wdt  = ((1 << (TxByte & 0x0F))+11) * vEtu;
						atr->Twdt = ((1 << (TxByte >> 4))+11) * 960 * vEtu;
						CAS_DBG(SMC,"ATR---->>Wdt:%d Twdt:%d\n",((1 << (TxByte & 0x0F))+11),((1 << (TxByte >> 4))+11)* 960);
					}
				}

				if (Y & (TCMASK)) { /* TC3 character present */   /* EMV 4.3.3.11  - ISO 9.5.4 */
					offset++;
					TxByte = rawatr[offset];
					CAS_DBG(SMC,"ATR---->>TC2:0x%02x\n",TxByte);
					if(globalSet) {
						/* T==15 for TD(2) so this byte is global - not T=1 specific */
						/* Ignore the parameters for now */
					} else {  /* T = 1 error detect check */
						if(rawatr[offset] != 0) {
							/* for EMV reject any other value than zero */
							//result = MMAC_SCARD_FAIL;
						} else if((rawatr[offset] & 0x1) == 0x1) {
							/* for ISO CRC mode is acceptable */
							//atrParams->EDC = MMAC_SCARD_CRC_EDC;
							CAS_DBG(SMC,"ATR---->>CRC Check\n");
						}
						CAS_DBG(SMC,"ATR---->>LRC Check\n"); /* default is LRC mode */
					}
				}

				if (Y & (TDMASK)) { /* TD3 character present */
					offset++;
					TxByte = rawatr[offset];
					CAS_DBG(SMC,"ATR---->>TD2:0x%02x\n",TxByte);
					{
						/* for ISO there may be multiple interface bytes defined for the same protocol */
						/* (do not use this data for now - just increment i as required) */
						do {
							Y = (uint8_t)(rawatr[offset] & ((uint8_t)0xF0));
							if(Y & (TDMASK))
								TDpresent = TRUE;
							else
								TDpresent = FALSE;
							Y = (Y >> 4);
							/* while Tx(i) chars present */
							while(Y != 0) {
								offset++;
								Y = (Y >> 0x01);
							}
						}while(TDpresent);
					}
				}
			}
		}
	}

	//T1->TK
	offset++;
	CAS_DBG(SMC,"ATR: HISTORICAL--->");
	for(i=0; i<K; i++)
		CAS_DBG(SMC,"%c", (rawatr+offset)[i]);
	CAS_DBG(SMC,"\n");

	if (vProtocol == GXSMC_PROTCL_T14) {
		if (strstr((const char *)(rawatr+offset), "IRDETO") != NULL ) {
			atr->Etu = vEtu = T14_RUNTIME_ETU;
			atr->baud_rate = T14_RUNTIME_BAUD_RATE;
			atr->Egt =  T14_RUNTIME_EGT;
			atr->Wdt  = T14_RUNTIME_WDT;
            atr->Tgt  = T14_RUNTIME_TGT;
            atr->Twdt  = T14_RUNTIME_TWDT;
		}
	}
	offset += K;

	atr->protocol = vProtocol;
	if(vProtocol == GXSMC_PROTCL_T0)
		return 0;
	//check for TCK character
	TxByte = rawatr[offset];       /* TCK character */
	CAS_DBG(SMC,"ATR---->>TCK:0x%02x\n",TxByte);
	offset++;
	if (offset != length)
		return -1;

	/* perform XOR check on ATR */
	TxByte = rawatr[1];
	for (offset = 2; offset < length; ++offset) {
		TxByte ^= rawatr[offset];
	}
	if (TxByte != 0) {/* check failed */
		CAS_DBG(SMC,"TCK XOR check error!\n");
		return -1;
	}

	return 0;
}

int32_t GxCas_Smc_AnalyseAtr(uint8_t *atr,uint32_t len, GxSmcTimeParams *time)
{
    CAS_DUMP(DEMUX ,atr ,len ,"SmartCard, Start analyse the ATR!!\n");
	ATR *analyse_atr = malloc(sizeof(ATR));
	if(NULL == analyse_atr || time == NULL || atr == NULL){
		CAS_ERR(SMC,"\nSmartCard, malloc failed\n");
		return -1;
	}
	if( (atr_parse(analyse_atr, atr, len) != 0)){
		if(analyse_atr)
			free(analyse_atr);
		analyse_atr = NULL;
        CAS_ERR(SMC,"atr_parse failed\n");
		return -1;
	}

	time->baud_rate = analyse_atr->baud_rate;
	time->egt = analyse_atr->Egt;
	time->tgt = analyse_atr->Tgt;
	time->twdt = analyse_atr->Twdt;
	time->wdt = analyse_atr->Wdt;
	time->etu = analyse_atr->Etu;
	time->protocol = analyse_atr->protocol;
	CAS_ERR(SMC,"\nbud = %d, egt = %d,tgt = %d,twdt = %d,wdt = %d,etu = %d, protocol = %d\n",\
        time->baud_rate,time->egt,time->tgt,time->twdt,time->wdt,time->etu, time->protocol);

	if(analyse_atr)
		free(analyse_atr);
	analyse_atr = NULL;
	return 0;
}
