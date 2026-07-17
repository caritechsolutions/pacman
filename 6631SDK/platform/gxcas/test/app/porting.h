#ifndef __APP_H__
#define __APP_H__

#define TAURUS_CHIP

typedef enum{
	GXCAS_TEST_PLAY,
	GXCAS_TEST_KEYUP,
	GXCAS_TEST_KEYDOWN,
	GXCAS_TEST_KEY0,
	GXCAS_TEST_KEY1,
	GXCAS_TEST_KEY2,
	GXCAS_TEST_KEY3,
	GXCAS_TEST_KEY4,
	GXCAS_TEST_KEY5,
	GXCAS_TEST_KEY6,
	GXCAS_TEST_KEY7,
	GXCAS_TEST_KEY8,
	GXCAS_TEST_KEY9,
}GxCas_Test_KeyMap;

void GxCas_CaTest_FrontendInit();
void GxCas_CaTest_GetTpParam(char *param);
void GxCas_CaTest_CaInit();
void GxCas_CaTest_KeyPress(GxCas_Test_KeyMap);
void GxCas_TestCa_Play(const char *url);
#endif
