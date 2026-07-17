//***********************************************
//RT500 software update: 2013/02/27
//file: header file
//version: v2.9
//***********************************************

#include "app.h"
#include "app_module.h"
#if ((RF_MODULATOR_SUPPORT > 0)&&(RF_RT500 >0))
#ifndef  _RT500_H_
#define _RT500_H_

#define UINT8 unsigned char
#define UINT16 unsigned int
#define UINT32 unsigned long

#define RT500_DEVICE_ADDRESS	0xCA
#define RT500_Xtal	16000
#define RT500_Reg_Num	11

typedef enum _RT500_Err_Type
{
	RT_Success = TRUE,
	RT_Fail    = FALSE
}RT500_Err_Type;

typedef enum _RT500_Standard_Type
{
	RT_Standard_M_N = 0,
	RT_Standard_B_G_H,
	RT_Standard_I,
	RT_Standard_D_K,
	RT_Standard_L
}RT500_Standard_Type;

typedef enum _RT500_Power_Type
{
	All_ON = 0,
	Mod_ON,
	LT_ON,
	Standby_ON
}RT500_Power_Type;

typedef enum _RT500_Video_Att
{
	Att_1=0,
	Att_0_75,
	Att_0_63,
	Att_0_5

}RT500_Video_Type;

typedef enum _RT500_Sound_Att
{
	Att_8k=0,
	Att_7k,
	Att_6k,
	Att_5k,
	Att_4k,
	Att_3k,
	Att_2k,
	Att_1k
}RT500_Sound_Type;

typedef enum _RT500_RF_Att
{
	RFAtt_0=0,  // 85.2 dBuv (highest)
	RFAtt_1,	// 83.2 dBuV
	RFAtt_2,	// 81.5 dBuV
	RFAtt_3,	// 78.6 dBuV
	RFAtt_4,	// 76.4 dBuV
	RFAtt_5,	// 73.3 dBuV
	RFAtt_6,	// 70.9 dBuV
	RFAtt_7     // 67.7 dBuv (lowest)
}RT500_RF_ATT_Type;

typedef enum _RT500_RF_ATT_6DB
{
	RFAtt_0DB=0,
	RFAtt_6DB    //attenuation extra 6dB
}RT500_RF_ATT_6DB_Type;

typedef struct _RT500_INFO_Type
{
	UINT32 RT500_Ch_Freq;
	RT500_Standard_Type RT500_Standard_Mode;
	RT500_Power_Type RT500_Power_Mode;
	RT500_Video_Type RT500_Video_Mode;
	RT500_Sound_Type RT500_Sound_Mode;
	RT500_RF_ATT_Type RT500_RF_Att_Mode;
	RT500_RF_ATT_6DB_Type RT500_RF_Att_6db_Mode;
}RT500_INFO_Type;

RT500_Err_Type RT500_Setting(RT500_INFO_Type RT500_Set_Info);
void RT500_Setup(UINT32 fre,int suond_mode);
void RT500_Standby(void);
#endif
#endif
