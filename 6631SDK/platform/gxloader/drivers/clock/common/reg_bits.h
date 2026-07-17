#ifndef __REG_BITS_H__
#define __REG_BITS_H__

extern void         xreg_set_val  (void *reg, unsigned int val);
extern void         xreg_set_field(void *reg, unsigned int val, unsigned int bits, unsigned int offset);
extern unsigned int xreg_get_val  (void *reg);
extern unsigned int xreg_get_field(void *reg, unsigned int bits, unsigned int offset);
extern void         xreg_set_bit  (void *reg, unsigned int offset);
extern void         xreg_clr_bit  (void *reg, unsigned int offset);

#define do_div(a,b) a = (a)/(b)
#define gxreg_set_val   xreg_set_val
#define gxreg_get_val   xreg_get_val
#define gxreg_set_field xreg_set_field
#define gxreg_get_field xreg_get_field
#define gxreg_set_bit   xreg_set_bit
#define gxreg_clr_bit   xreg_clr_bit

#endif
