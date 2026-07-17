#include "gxtype.h"
#include "autoconf.h"
#include "gxcas_dbg.h"
#include "gxcas_service.h"
#include "gxcas_queue.h"
#include "gxcas_smartcard.h"
#include "cryptoguard_misc.h"

#ifdef CONFIG_CARD
static CPG_CARD_STATUS s_CPGCardStatus = CPG_CARD_FAIL;
#endif
/* 智能卡复位 */
uint8_t CPGSmcReset(void )
{
#define RUNTIME_ETU                         (372)
#define RUNTIME_BAUD_RATE                   (9600*RUNTIME_ETU)
#define RUNTIME_TGT                         (0)
#define RUNTIME_TWDT                        (3*9600 * RUNTIME_ETU)

#define RUNTIME_EGT                         (RUNTIME_ETU * 20)
#define RUNTIME_WDT                         (45 * RUNTIME_ETU)

	uint8_t atr[256] = {0};
	GxSmcTimeParams    time;
	int32_t len = 0;
	int count = 1;
	time.baud_rate  = RUNTIME_BAUD_RATE;
	time.egt        = RUNTIME_EGT;
	time.etu        = RUNTIME_ETU;
	time.tgt        = RUNTIME_TGT;
	time.twdt       = RUNTIME_TWDT;
	time.wdt        = RUNTIME_WDT;

SMC_RESET:
	if (GxCas_Smc_Reset(atr, 256, &len) > 0) {
#ifdef CONFIG_SCI_NEW
	        GxCas_Smc_SetMode(GXSMC_MODE_BOOST);
		if (GxCas_Smc_CACardSetup(atr, len) == 0)
#else
		if (GxCas_Smc_Config(&time) == 0)
#endif
            return TRUE;
	} else if (count > 0) {
		count --;
		goto SMC_RESET;
	}

	return FALSE;
}
void cpg_smartcard_change(uint32_t flag)
{
#ifdef CONFIG_CARD
	if (flag){
		if(CPGSmcReset())
			s_CPGCardStatus = CPG_CARD_OK;
		else
			s_CPGCardStatus = CPG_CARD_FAIL;
		GxCas_QueuePut(GXCAS_CRYPTOGUARD_CARD_IN, NULL, 0, 0);
	} else {
		s_CPGCardStatus = CPG_CARD_FAIL;
		GxCas_QueuePut(GXCAS_CRYPTOGUARD_CARD_OUT, NULL, 0, 0);
	}
#endif
}

#ifdef CONFIG_CARD
int32_t cpg_smartcard_ready(void)
{
	if (s_CPGCardStatus == CPG_CARD_OK)
		return TRUE;
	else 
		return FALSE;
}
#endif

