#include "gxcore.h"
#include "../../include/gxcas_smartcard.h"
#include "../../include/gxcas_dbg.h"

static int get_atr(void)
{
	unsigned char atr[256];
	int i,len;

	GxCas_Smc_Reset(&atr[0],256,&len);

	printf("ATR %d [ ",len);
	for(i=0;i<len;i++)
		printf("0x%x ",atr[i]);

	printf("]\n");
	GxCas_Smc_Config_ByAtr(&atr[0],len);
	return 0;

}

static int one_cmd(void)
{
	unsigned char cmd[64]={0x0, 0x16, 0x0, 0x1, 0x4, 0x1, 0xc4, 0x0, 0x0};
	unsigned char reply[64]={0};
	unsigned int i,len;

	GxCas_Smc_Apdu(&cmd[0], 9, &reply[0], &len);
	return 0;

}
static void sci_callback(unsigned int flag)
{
	if(flag == 1){
		printf("+++++ GXSMART_CARD_INIT +++++\n");
		get_atr();
//		one_cmd();
	} else if(flag == 0){
		printf("+++++ GXSMART_CARD_OUT +++++\n");
	}
}

static int sci_init(void)
{

	GxSmcParams param = {0};
	GxCas_Smc_Init( GXCAS_SMC_HIGH_LEVEL, GXCAS_SMC_LOW_LEVEL);
	param.io_conv = GXSMC_DATA_CONV_DIRECT;
	param.parity = GXSMC_PARITY_ODD;// GXSMC_PARITY_EVEN;
	param.protocol = DISABLE_REPEAT_WHEN_ERR;
	param.stop_len = GXSMC_STOPLEN_0BIT;
	param.default_etu = 372;
	GxCas_Smc_Open(&param,  sci_callback);

	return 0;
}


static void ca_task(void* arg)
{
	sci_init();
	while (1) {
		GxCore_ThreadDelay(1000);
	}
}
int GxCore_Startup(int argc, char **argv)
{
	static int handle;

	GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_CAS);
	GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_SMC);
	GxCore_ThreadCreate("ca", &handle, ca_task, NULL,30*1024,GXOS_DEFAULT_PRIORITY);
	GxCore_Loop();
	return 0;
}
