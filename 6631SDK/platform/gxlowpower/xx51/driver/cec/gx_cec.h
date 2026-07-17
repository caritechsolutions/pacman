#ifndef __GX_CEC_H__
#define __GX_CEC_H__

extern void cec_init(void);
extern void cec_suspend(void);
extern void cec_resume(void);
extern unsigned char cec_get_tv_resume_flag(void);
extern void cec_set_tv_resume_flag(unsigned char flag);
extern void cec_ISR(void) __interrupt 2; //int1_n ISR

#endif
