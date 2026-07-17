#ifndef __GX_IRR_H__
#define __GX_IRR_H__

typedef enum
{
	IRR_NOT_SURE = 0,
	IRR_NEC,
	IRR_PHILIPS_RC5,
	IRR_PANASONIC,
	IRR_40BIT,
	IRR_BESCON
}IRR_PROTOCOL_TYPE;

typedef struct
{
	volatile unsigned long key_code;
	volatile unsigned char key_protocol;
	volatile unsigned char key_code_valid; //key_code是否为有效值
	volatile unsigned long irr_32bit_data;
	volatile unsigned char pulse_num;
	volatile unsigned char protocol;
	volatile unsigned int pulse_width;
}irr_parse_t;

extern void int0_n_ISR(void) __interrupt 0;
extern void timer0_ISR(void) __interrupt 1;
#endif /*__GX_IRR_H__*/
