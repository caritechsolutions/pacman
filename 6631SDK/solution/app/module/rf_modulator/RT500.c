/****************************************************************
                           RT500.c
Copyright 2011 by Rafaelmicro., Inc.
                 C H A N G E   R E C O R D
  Date        Author           Version      Description
--------   --------------    --------   ----------------------
12/06/2011  Cloudio & Wig       2.0    Module released
12/13/2011  Cloudio				2.1	   LP1=on, RF_ATT=-15dB (for all freq)
03/10/2012  Jason				2.2	   Temporary for RT500P
03/23/2012  Cloudio				2.3	   RT500P First Update
04/02/2012  Cloudio				2.4	   Wite_Clip_Level = 000,  Modulataion Depth code -3
05/04/2012  Jason				2.5	   Wite_Clip_Level = 001,  Modulataion Depth+1, Xtal Power=High(R8[7]=0)
09/20/2012  Jason				2.6	   Analog LDO=2.55v,  VCO power=000(max)
09/21/2012  Jason				2.7	   set PLL Manual then Auto
10/12/2012  Jason               2.8    set VCO power
02/27/2013	Ryan				2.9	   updata Sound Filter Corner(Register[8] = RT500_Reg_Arry[4])
****************************************************************/
//#include "stdafx.h"
//#include "..\I2C_Sys.h"
#include "app.h"

#if ((RF_MODULATOR_SUPPORT > 0)&&(RF_RT500 >0))
#include "RT500.h"
typedef struct _I2C_LEN_TYPE
{
    UINT8 RegAddr;
    UINT8 Data[50];
    UINT8 Len;
} I2C_LEN_TYPE;

typedef struct _I2C_TYPE
{
    UINT8 RegAddr;
    UINT8 Data;
} I2C_TYPE;

I2C_TYPE RT500_Write_Byte;
I2C_LEN_TYPE RT500_Write_Len;

RT500_Err_Type RT500_PLL(UINT32 PLL_Freq);

UINT8 RT500_Reg_Arry[RT500_Reg_Num] = {0x00, 0x88, 0x98, 0x04, 0x59, 0x4B, 0xE8, 0x70, 0x06, 0x2F, 0x08};
// 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E

RT500_INFO_Type RT500_Set_Info = {0};

int App_R828_Convert(int InvertNum)
{
    int ReturnNum = 0;
    int AddNum    = 0x80;
    int BitNum    = 0x01;
    int CuntNum   = 0;

    for (CuntNum = 0; CuntNum < 8; CuntNum ++)
    {
        if (BitNum & InvertNum)
            ReturnNum += AddNum;

        AddNum /= 2;
        BitNum *= 2;
    }

    return ReturnNum;
}
bool App_I2C_Write_Len(I2C_LEN_TYPE *I2C_Info)
{
#define DEMOD_I2C_ID    0
#define DEMOD_I2C_ADDR    0xCA
#define DEMOD_REG_CHIPID_H    0x00
#define DEMOD_REG_CHIPID_L    0x00
#define DEMOD_REG_RST    0x30
#define DEMOD_RST_CMD    1

    I2cSetParam i2c_set = {0};
    I2cUserData data = {0};
    int ret = 0;

    //get demod chip id
    i2c_set.chip_address = DEMOD_I2C_ADDR;
    i2c_set.address_width = 1;
    i2c_set.send_noack = 0;
    i2c_set.send_stop = 0;

    data.count = 11;
    data.rtx_data = I2C_Info->Data;

    for (ret = 0 ; ret < data.count; ret++)
        app_log_info("0x%x ", data.rtx_data[ret]);
    app_log_info("\nwrite reg[0x%x] end======\n", I2C_Info->RegAddr);

    ret = app_gxi2c_tx(DEMOD_I2C_ID, I2C_Info->RegAddr, &i2c_set, &data);

    app_log_info("~~~~~~~~~~[%s] ret = %d,I2C_Info->Len=%d~~~~~~~~~~~~\n", __func__, ret, I2C_Info->Len);
    if (ret == data.count)
        return true;
    else
        return false;
}

bool App_I2C_Read_Len(I2C_LEN_TYPE *I2C_Info)
{
#define DEMOD_I2C_ID    0
#define DEMOD_I2C_ADDR    0xCA
#define DEMOD_REG_RST    0x30
#define DEMOD_RST_CMD    1

    I2cSetParam i2c_set = {0};
    I2cUserData data = {0};
    int 		ret = 0;
    uint8_t val[15] = {0};

    //get demod chip id
    i2c_set.chip_address = DEMOD_I2C_ADDR;
    i2c_set.address_width = 1;
    i2c_set.send_noack = 0;
    i2c_set.send_stop = 1;

    data.count = 15;
    data.rtx_data = (unsigned char *)&val;
    ret = app_gxi2c_rx(DEMOD_I2C_ID, I2C_Info->RegAddr, &i2c_set, &data);

    app_log_info("~~~~~~~~~~[%s] ret = %d count=%d ~~~~~~~~~~~~\n", __func__, ret, data.count);
    if (data.count == ret)
    {
        for (ret = 4; ret < data.count; ret++)
            app_log_info(" 0x%x ", App_R828_Convert(val[ret]));
        app_log_info("\n");
        return true;
    }
    else
        return false;
}

RT500_Err_Type RT500_Setting(RT500_INFO_Type RT500_Set_Info)
{
    UINT8 RT500_Reg_Cunt = 0;

    RT500_Reg_Arry[9] &= 0xF8;
    switch (RT500_Set_Info.RT500_Power_Mode)
    {
        case All_ON:
            RT500_Reg_Arry[0] = 0x00;
            RT500_Reg_Arry[1] = 0x88;
            RT500_Reg_Arry[9] |= 0x07;
            break;
        case Mod_ON:
            RT500_Reg_Arry[0] = 0x00;
            RT500_Reg_Arry[1] = 0xA8;
            RT500_Reg_Arry[9] |= 0x07;
            break;
        case LT_ON:
            RT500_Reg_Arry[0] = 0xFE;
            RT500_Reg_Arry[1] = 0x9B;
            RT500_Reg_Arry[9] |= 0x06;
            break;
        case Standby_ON:
            RT500_Reg_Arry[0] = 0xFF;
            RT500_Reg_Arry[1] = 0xBF;
            RT500_Reg_Arry[9] |= 0x06;
            break;
        default:
            RT500_Reg_Arry[0] = 0x00;
            RT500_Reg_Arry[1] = 0x88;
            RT500_Reg_Arry[9] |= 0x07;
    }

    //Set Standard Mode for RT500
    RT500_Reg_Arry[2] &= 0xF0;
    RT500_Reg_Arry[3] &= 0x87;
    switch (RT500_Set_Info.RT500_Standard_Mode)
    {
        case RT_Standard_M_N:
            RT500_Reg_Arry[2] |= 0x08;  //r6
            RT500_Reg_Arry[3] |= 0x00;  //r7
            RT500_Reg_Arry[4] =  0x41;  //r8
            break;

        case RT_Standard_B_G_H:
            RT500_Reg_Arry[2] |= 0x08;
            RT500_Reg_Arry[3] |= 0x30;
            RT500_Reg_Arry[4] =  0x21;
            break;

        case RT_Standard_I:
            RT500_Reg_Arry[2] |= 0x08;
            RT500_Reg_Arry[3] |= 0x58;
            RT500_Reg_Arry[4] =  0x11;
            break;

        case RT_Standard_D_K:
            RT500_Reg_Arry[2] |= 0x08;
            RT500_Reg_Arry[3] |= 0x78;
            RT500_Reg_Arry[4] =  0x01;
            break;

        case RT_Standard_L:
            RT500_Reg_Arry[2] |= 0x05;
            RT500_Reg_Arry[3] |= 0x78;
            RT500_Reg_Arry[4] =  0x00;
            break;

        default:  //DK
            RT500_Reg_Arry[2] |= 0x08;
            RT500_Reg_Arry[3] |= 0x78;
            RT500_Reg_Arry[4] =  0x01;
            break;
    }

    //Set Input Video Swing
    RT500_Reg_Arry[10] &= 0xFC;
    switch (RT500_Set_Info.RT500_Video_Mode)
    {
        case Att_1:
            RT500_Reg_Arry[10] |= 0x00;
            break;
        case Att_0_75:
            RT500_Reg_Arry[10] |= 0x01;
            break;
        case Att_0_63:
            RT500_Reg_Arry[10] |= 0x02;
            break;
        case Att_0_5:
            RT500_Reg_Arry[10] |= 0x03;
            break;
        default:
            RT500_Reg_Arry[10] |= 0x00;
            break;
    }

    //Set Input Audio Swing
    RT500_Reg_Arry[3] &= 0xF8;
    switch (RT500_Set_Info.RT500_Sound_Mode)
    {
        case Att_8k:
            RT500_Reg_Arry[3] |= 0x00;
            break;
        case Att_7k:
            RT500_Reg_Arry[3] |= 0x01;
            break;
        case Att_6k:
            RT500_Reg_Arry[3] |= 0x02;
            break;
        case Att_5k:
            RT500_Reg_Arry[3] |= 0x03;
            break;
        case Att_4k:
            RT500_Reg_Arry[3] |= 0x04;
            break;
        case Att_3k:
            RT500_Reg_Arry[3] |= 0x05;
            break;
        case Att_2k:
            RT500_Reg_Arry[3] |= 0x06;
            break;
        case Att_1k:
            RT500_Reg_Arry[3] |= 0x07;
            break;
        default:
            RT500_Reg_Arry[3] |= 0x04;
            break;
    }

    //set RF ATT (RF output level)
    RT500_Reg_Arry[8] &= 0xF8;
    //RT500_Reg_Arry[8] |= RT500_Set_Info.RT500_RF_Att_Mode;
    switch (RT500_Set_Info.RT500_RF_Att_Mode)
    {
        case RFAtt_0:
            RT500_Reg_Arry[8] |= 0x00;
            break;
        case RFAtt_1:
            RT500_Reg_Arry[8] |= 0x01;
            break;
        case RFAtt_2:
            RT500_Reg_Arry[8] |= 0x02;
            break;
        case RFAtt_3:
            RT500_Reg_Arry[8] |= 0x03;
            break;
        case RFAtt_4:
            RT500_Reg_Arry[8] |= 0x04;
            break;
        case RFAtt_5:
            RT500_Reg_Arry[8] |= 0x05;
            break;
        case RFAtt_6:
            RT500_Reg_Arry[8] |= 0x06;
            break;
        case RFAtt_7:
            RT500_Reg_Arry[8] |= 0x07;
            break;
        default:
            RT500_Reg_Arry[8] |= 0x00;
            break;
    }

    //set RF_ATT_6dB mode
    RT500_Reg_Arry[8] &= 0xF7;
    switch (RT500_Set_Info.RT500_RF_Att_6db_Mode)
    {
        case RFAtt_0DB:
            RT500_Reg_Arry[8] |= 0x00;
            break;
        case RFAtt_6DB:
            RT500_Reg_Arry[8] |= 0x08;
            break;
        default:
            RT500_Reg_Arry[8] |= 0x00;
            break;
    }

    //set modulation depth ~80%			//update depend on datasheet v2.7
    RT500_Reg_Arry[2] &= 0x0F;
    if ((RT500_Set_Info.RT500_Ch_Freq > 0) && (RT500_Set_Info.RT500_Ch_Freq < 225000))
        RT500_Reg_Arry[2] |= 0xA0;
    else if ((RT500_Set_Info.RT500_Ch_Freq >= 225000) && (RT500_Set_Info.RT500_Ch_Freq < 450000))
        RT500_Reg_Arry[2] |= 0x90;
    else if ((RT500_Set_Info.RT500_Ch_Freq >= 450000) && (RT500_Set_Info.RT500_Ch_Freq < 650000))
        RT500_Reg_Arry[2] |= 0x80;
    else if ((RT500_Set_Info.RT500_Ch_Freq >= 650000) && (RT500_Set_Info.RT500_Ch_Freq < 750000))
        RT500_Reg_Arry[2] |= 0x70;
    else
        RT500_Reg_Arry[2] |= 0x60;

    //Set Frequency for RT500
    if (RT500_PLL(RT500_Set_Info.RT500_Ch_Freq) != RT_Success)
    {
        app_log_error("===RT500_PLL Fail====\n");
        return RT_Fail;
    }

    RT500_Write_Len.RegAddr = 0x04;
    for (RT500_Reg_Cunt = 0; RT500_Reg_Cunt < RT500_Reg_Num; RT500_Reg_Cunt ++)
    {
        RT500_Write_Len.Data[RT500_Reg_Cunt] = RT500_Reg_Arry[RT500_Reg_Cunt];
    }
    RT500_Write_Len.Len = RT500_Reg_Num;
    if (App_I2C_Write_Len(&RT500_Write_Len) != RT_Success)
        return RT_Fail;

    return RT_Success;
}

RT500_Err_Type RT500_PLL(UINT32 PLL_Freq)
{
    UINT8 Div_Rat = 2;
    UINT8 Sel_Div = 0;
    UINT8 Ni = 0;
    UINT8 Si = 0;
    UINT8 N1 = 0;
    UINT8 N2 = 0;
    UINT32 VCO_Min = 1800000;
    UINT32 VCO_Freq = 0;

    while (Div_Rat < 64)
    {
        Div_Rat *= 2;
        if ((PLL_Freq * Div_Rat) >= VCO_Min)
        {

            RT500_Reg_Arry[7] &= 0xF8;
            RT500_Reg_Arry[7] = Sel_Div  | RT500_Reg_Arry[7]; // r11
            VCO_Freq = PLL_Freq * Div_Rat;

            break;
        }
        Sel_Div ++;
    }

    N1 = (UINT8)(((VCO_Freq / 1000) - 416) / 32);
    Ni = (UINT8)(N1 / 4);
    Si = (UINT8)(N1 - (Ni * 4));
    RT500_Reg_Arry[5] = Ni | (Si << 6); // r9
    N2 = (UINT8)(((VCO_Freq / 1000) - 416) - (N1 * 32));
    if (N2 == 0)
        RT500_Reg_Arry[7] |= 0x08;
    else
        RT500_Reg_Arry[7] &= 0xF7;
    RT500_Reg_Arry[6] = (N2 << 3);  // r10

    return RT_Success;
}

void RT500_Setup(UINT32 fre, int suond_mode)
{
    RT500_Set_Info.RT500_Ch_Freq = fre;
    RT500_Set_Info.RT500_Power_Mode = Mod_ON;
    RT500_Set_Info.RT500_Standard_Mode = suond_mode;
    RT500_Set_Info.RT500_RF_Att_6db_Mode = RFAtt_0DB;
    RT500_Set_Info.RT500_RF_Att_Mode = RFAtt_2;
    RT500_Set_Info.RT500_Video_Mode = Att_1;
    RT500_Set_Info.RT500_Sound_Mode = Att_8k;
    if (RT_Success == RT500_Setting(RT500_Set_Info))
        app_log_info("\n==============RT500_Setup success!!==================\n");
    else
        app_log_error("\n==============RT500_Setup fail!!==================\n");
}

void RT500_Standby(void)
{
    RT500_Set_Info.RT500_Power_Mode = Standby_ON;
    if (RT_Success == RT500_Setting(RT500_Set_Info))
        app_log_info("\n==============RT500_Standby success!!==================\n");
    else
        app_log_error("\n==============RT500_Standby fail!!==================\n");
}

void Read_Test(void)
{
    I2C_LEN_TYPE *I2C_info = NULL;
    I2C_info->RegAddr = 0x00;

    if (true != App_I2C_Read_Len(I2C_info))
    {
        app_log_error("=============RT500 Read ERROR!!================");
    }
}
#endif
