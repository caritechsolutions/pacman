#ifndef __NC_INTC_H__
#define __NC_INTC_H__

/*
 * Base Address : interrupt
 */

#define INTC_NINT            0x00
#define INTC_NINT2           0x04
#define INTC_NPEND           0x10
#define INTC_NPEND2          0x14
#define INTC_NENSET          0x20
#define INTC_NENSET2         0x24
#define INTC_NENCLR          0x30
#define INTC_NENCLR2         0x34
#define INTC_NEN             0x40
#define INTC_NEN2            0x44
#define INTC_NMASK           0x50
#define INTC_NMASK2          0x54

#define INTC_NSOURCE03_00    0x60
#define INTC_NSOURCE07_04    0x64
#define INTC_NSOURCE11_08    0x68
#define INTC_NSOURCE15_12    0x6c
#define INTC_NSOURCE19_16    0x70
#define INTC_NSOURCE23_20    0x74
#define INTC_NSOURCE27_24    0x78
#define INTC_NSOURCE31_28    0x7c
#define INTC_NSOURCE35_32    0x80
#define INTC_NSOURCE39_36    0x84
#define INTC_NSOURCE43_40    0x88
#define INTC_NSOURCE47_44    0x8c
#define INTC_NSOURCE51_48    0x90
#define INTC_NSOURCE55_52    0x94
#define INTC_NSOURCE59_56    0x98
#define INTC_NSOURCE63_60    0x9c

#define INTC_FINT            0x100
#define INTC_FINT2           0x104
#define INTC_FPEND           0x110
#define INTC_FPEND2          0x114
#define INTC_FENSET          0x120
#define INTC_FENSET2         0x124
#define INTC_FENCLR          0x130
#define INTC_FENCLR2         0x134
#define INTC_FEN             0x140
#define INTC_FEN2            0x144
#define INTC_FMASK           0x150
#define INTC_FMASK2          0x154

#define INTC_FSOURCE03_00    0x160
#define INTC_FSOURCE07_04    0x164
#define INTC_FSOURCE11_08    0x168
#define INTC_FSOURCE15_12    0x16c
#define INTC_FSOURCE19_16    0x170
#define INTC_FSOURCE23_20    0x174
#define INTC_FSOURCE27_24    0x178
#define INTC_FSOURCE31_28    0x17c
#define INTC_FSOURCE35_32    0x180
#define INTC_FSOURCE39_36    0x184
#define INTC_FSOURCE43_40    0x188
#define INTC_FSOURCE47_44    0x18c
#define INTC_FSOURCE51_48    0x190
#define INTC_FSOURCE55_52    0x194
#define INTC_FSOURCE59_56    0x198
#define INTC_FSOURCE63_60    0x19c


#endif
