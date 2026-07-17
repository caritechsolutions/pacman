/*************************************************
  Copyright (C), 2017-2018, GX SOC.
  File name :  reg_dw8051.h
  Author    :  luyg
  Version   :  1.0
  Date      :  2017,02,14
  Description: Header file for generic DW8051 microcontroller.
  Others     :
  History    :
    1. Date:       Author:        Modification:
    2. ...
*************************************************/

#ifndef __REG_DW8051_H__
#define __REG_DW8051_H__

/*  BYTE Register  */
__sfr __at (0x81) SP;
__sfr __at (0x82) DPL0;
__sfr __at (0x83) DPH0;
__sfr __at (0x84) DPL1;
__sfr __at (0x85) DPH1;
__sfr __at (0x86) DPS;
__sfr __at (0x87) PCON;
__sfr __at (0x88) TCON;
__sfr __at (0x89) TMOD;
__sfr __at (0x8A) TL0;
__sfr __at (0x8B) TL1;
__sfr __at (0x8C) TH0;
__sfr __at (0x8D) TH1;
__sfr __at (0x8E) CKCON;
__sfr __at (0x8F) SPC_FNC;
__sfr __at (0x91) EXIF;
__sfr __at (0x92) MPAGE;
__sfr __at (0x98) SCON0;
__sfr __at (0x99) SBUF0;
__sfr __at (0xA8) IE;
__sfr __at (0xB8) IP;
__sfr __at (0xC0) SCON1;
__sfr __at (0xC1) SBUF1;
__sfr __at (0xC8) T2CON;
__sfr __at (0xCA) RCAP2L;
__sfr __at (0xCB) RCAP2H;
__sfr __at (0xCC) TL2;
__sfr __at (0xCD) TH2;
__sfr __at (0xD0) PSW;
__sfr __at (0xD8) EICON;
__sfr __at (0xE0) ACC;
__sfr __at (0xE8) EIE;
__sfr __at (0xF0) B;
__sfr __at (0xF8) EIP;

__sfr __at (0x80) GPIO_P0;
__sfr __at (0x90) GPIO_P1;
__sfr __at (0xA0) GPIO_P2;
__sfr __at (0xB0) GPIO_P3;
__sfr __at (0xe9) mcu_config_reg;
__sfr __at (0x93) mcu_control_signal;
__sfr __at (0x94) pcb_power_on_time_para;

__sfr __at (0x9A) mcu_gpio_funsel00_l;
__sfr __at (0x9B) mcu_gpio_funsel01_l;
__sfr __at (0x9C) mcu_gpio_funsel10_l;
__sfr __at (0x9D) mcu_gpio_funsel11_l;
__sfr __at (0x9E) mcu_gpio_funsel00_h;
__sfr __at (0x9F) mcu_gpio_funsel01_h;
__sfr __at (0xC9) mcu_gpio_funsel10_h;
__sfr __at (0xA1) mcu_gpio_funsel11_h;

__sfr __at (0xCE) mcu_rd_fsm_state;
__sfr __at (0xD1) driver_ctrl00;
__sfr __at (0xD2) driver_ctrl01;
__sfr __at (0xD3) driver_ctrl10;
__sfr __at (0xD4) driver_ctrl11;
__sfr __at (0xAB) MCU_INT1;
__sfr __at (0xAC) MCU_INT1_EN;
__sfr __at (0xAD) MCU_INT2;
__sfr __at (0xAE) MCU_INT2_EN;
 /* GPIO_P0 80 */
__sbit __at (0x87) GPIO_P07;
__sbit __at (0x86) GPIO_P06;
__sbit __at (0x85) GPIO_P05;
__sbit __at (0x84) GPIO_P04;
__sbit __at (0x83) GPIO_P03;
__sbit __at (0x82) GPIO_P02;
__sbit __at (0x81) GPIO_P01;
__sbit __at (0x80) GPIO_P00;

 /* GPIO_P1 90 */
__sbit __at (0x97) GPIO_P17;
__sbit __at (0x96) GPIO_P16;
__sbit __at (0x95) GPIO_P15;
__sbit __at (0x94) GPIO_P14;
__sbit __at (0x93) GPIO_P13;
__sbit __at (0x92) GPIO_P12;
__sbit __at (0x91) GPIO_P11;
__sbit __at (0x90) GPIO_P10;

 /* GPIO_P2 a0 */
__sbit __at (0xA7) GPIO_P27;
__sbit __at (0xA6) GPIO_P26;
__sbit __at (0xA5) GPIO_P25;
__sbit __at (0xA4) GPIO_P24;
__sbit __at (0xA3) GPIO_P23;
__sbit __at (0xA2) GPIO_P22;
__sbit __at (0xA1) GPIO_P21;
__sbit __at (0xA0) GPIO_P20;

 /* GPIO_P3 b0 */
__sbit __at (0xB7) GPIO_P37;
__sbit __at (0xB6) GPIO_P36;
__sbit __at (0xB5) GPIO_P35;
__sbit __at (0xB4) GPIO_P34;
__sbit __at (0xB3) GPIO_P33;
__sbit __at (0xB2) GPIO_P32;
__sbit __at (0xB1) GPIO_P31;
__sbit __at (0xB0) GPIO_P30;



/*  BIT Register  */
/*  PSW   */
__sbit __at (0xD7) CY;
__sbit __at (0xD6) AC;
__sbit __at (0xD5) F0;
__sbit __at (0xD4) RS1;
__sbit __at (0xD3) RS0;
__sbit __at (0xD2) OV;
__sbit __at (0xD0) P;


/*  TCON  */
__sbit __at (0x8F) TF1;
__sbit __at (0x8E) TR1;
__sbit __at (0x8D) TF0;
__sbit __at (0x8C) TR0;
__sbit __at (0x8B) IE1;
__sbit __at (0x8A) IT1;
__sbit __at (0x89) IE0;
__sbit __at (0x88) IT0;

/*  SCON0  */
__sbit __at (0x9F) SM0_0;
__sbit __at (0x9E) SM1_0;
__sbit __at (0x9D) SM2_0;
__sbit __at (0x9C) REN_0;
__sbit __at (0x9B) TB8_0;
__sbit __at (0x9A) RB8_0;
__sbit __at (0x99) TI_0;
__sbit __at (0x98) RI_0;

/*  IE   */
__sbit __at (0xAF) EA;
__sbit __at (0xAE) ES1;
__sbit __at (0xAD) ET2;
__sbit __at (0xAC) ES0;
__sbit __at (0xAB) ET1;
__sbit __at (0xAA) EX1;
__sbit __at (0xA9) ET0;
__sbit __at (0xA8) EX0;

/*  IP   */
__sbit __at (0xBE) PS1;
__sbit __at (0xBD) PT2;
__sbit __at (0xBC) PS0;
__sbit __at (0xBB) PT1;
__sbit __at (0xBA) PX1;
__sbit __at (0xB9) PT0;
__sbit __at (0xB8) PX0;

/*  SCON1  */
__sbit __at (0xC0) SM0_1;
__sbit __at (0xC0) SM1_1;
__sbit __at (0xC0) SM2_1;
__sbit __at (0xC0) REN_1;
__sbit __at (0xC0) TB8_1;
__sbit __at (0xC0) RB8_1;
__sbit __at (0xC0) TI_1;
__sbit __at (0xC0) RI_1;

/*  T2CON */
__sbit __at (0xCF) TF2;
__sbit __at (0xCE) EXF2;
__sbit __at (0xCD) RCLK;
__sbit __at (0xCC) TCLK;
__sbit __at (0xCB) EXEN2;
__sbit __at (0xCA) TR2;
__sbit __at (0xC9) C_T2;
__sbit __at (0xC8) CP_RL2;

/*  EICON */
__sbit __at (0xDF) SMOD1;
__sbit __at (0xDD) EPFI;
__sbit __at (0xDC) PFI;
__sbit __at (0xDB) WDTI;

/*  EIE   */
__sbit __at (0xEC) EWDI;
__sbit __at (0xEB) EX5;
__sbit __at (0xEA) EX4;
__sbit __at (0xE9) EX3;
__sbit __at (0xE8) EX2;

/*  EIP   */
__sbit __at (0xFC) PWDI;
__sbit __at (0xFB) PX5;
__sbit __at (0xFA) PX4;
__sbit __at (0xF9) PX3;
__sbit __at (0xF8) PX2;

#endif
