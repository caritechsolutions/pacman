#include "gxcore.h"
#include "porting.h"
#include "bus/module/player/gxplayer_module.h"
#include "gxcas/gxcas.h"

#define TEST_WITDH          1920
#define TEST_HEIGHT         1080
#define CHECK_TIMEOUT_MS    5000

extern unsigned int cyg_time_get_ms(void);

static bool tp_lock(char *param)
{
	GxTime old = {0}, new = {0};

	GxCore_GetTickTime(&old);
	while(1) {
		if(0 == check_lock(param)){
			printf("lock success\n");
			return true;
		}else{
			GxCore_GetTickTime(&new);
			if ((new.microsecs - old.microsecs) >= CHECK_TIMEOUT_MS) {
				printf("error: %s(%d) timeout\n", __func__, __LINE__);
				return false;
			}
		}
	}

}

static void sys_init(void)
{
	fe_init();
	play_init();
	vout_init(0);
	aout_init(0);
	tdt_time_init();
	vpu_init(TEST_WITDH, TEST_HEIGHT);
}

static void ca_task(void* arg)
{
	char param[128] = {};

	sys_init();
	GxCas_CaTest_FrontendInit();
	GxCas_CaTest_GetTpParam(param);
#ifndef TAURUS_CHIP
	tp_lock(param);
#endif
	GxCas_CaTest_CaInit();
	GxCas_CaTest_KeyPress(GXCAS_TEST_PLAY);
	extern int remote_init();
	remote_init();

	while (1) {
		GxCore_ThreadDelay(1000);
	}
}

int check_status = 0;
int cw_status = 0;
void GxCas_TestCa_Play(const char *url)
{
	play(url);
	check_status = 1;
	cw_status = 1;
}

static void play_check(void *arg)
{
	while(1) {
		if (check_status == 0) {
			GxCore_ThreadDelay(10);
			continue;
		}
#if 0	
		extern GxTime abv_time;
		PlayerStatusInfo info;
		GxPlayer_MediaGetStatus("gxcasplayer", &info);
		
		if (info.vcodec.state == AVCODEC_RUNNING) {
			GxTime time = {0};
			GxCore_GetTickTime(&time);
			printf("%s,%d,%d\n", __func__, __LINE__,
				(time.seconds * 1000 * 1000 + time.microsecs) - (abv_time.seconds * 1000 * 1000 + abv_time.microsecs));
			check_status = 0;
		}
#endif
		//if (info.acodec.state == AVCODEC_RUNNING) {
		//	GxTime time = {0};
		//	GxCore_GetTickTime(&time);
		//	printf("%s,%d,%d\n", __func__, __LINE__,
		//		(time.seconds * 1000 * 1000 + time.microsecs) - (abv_time.seconds * 1000 * 1000 + abv_time.microsecs));
		//	check_status = 0;
		//}
		GxCore_ThreadDelay(10);
	}
}

int GxCore_Startup(int argc, char **argv)
{
	static int handle;
	static int handle2;
	printf("----CA TEST\n");

	GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_CAS);
	GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_FLASH);
	GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_SMC);
	GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_CASLIB);
	GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_DEMUX);
	GxCas_Enable_Debug(GXCAS_DEBUG_MOUDULE_SI);
	GxCore_ThreadCreate("ca", &handle, ca_task, NULL,30*1024,GXOS_DEFAULT_PRIORITY);
	GxCore_ThreadCreate("check", &handle2, play_check, NULL,30*1024,GXOS_DEFAULT_PRIORITY);
	GxCore_Loop();
	return 0;
}
