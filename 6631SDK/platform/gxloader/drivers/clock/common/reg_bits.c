#include "reg_bits.h"

static unsigned int _xreg_mask[33] = {
	0x00000000,
	0x00000001, 0x00000003, 0x00000007, 0x0000000f,
	0x0000001f, 0x0000003f, 0x0000007f, 0x000000ff,
	0x000001ff, 0x000003ff, 0x000007ff, 0x00000fff,
	0x00001fff, 0x00003fff, 0x00007fff, 0x0000ffff,
	0x0001ffff, 0x0003ffff, 0x0007ffff, 0x000fffff,
	0x001fffff, 0x003fffff, 0x007fffff, 0x00ffffff,
	0x01ffffff, 0x03ffffff, 0x07ffffff, 0x0fffffff,
	0x1fffffff, 0x3fffffff, 0x7fffffff, 0xffffffff
};

void xreg_set_val(void *reg, unsigned int val)
{
	(*(volatile unsigned int *)reg) = val;
}

unsigned int xreg_get_val(void *reg)
{
	return *((volatile unsigned int *)reg);
}

void xreg_set_bit(void *reg, unsigned int offset)
{
	unsigned int tmp_val = *(volatile unsigned int *)reg;

	tmp_val |= (0x1 << offset);
	(*(volatile unsigned int *)reg) = tmp_val;
}

void xreg_clr_bit(void *reg, unsigned int offset)
{
	unsigned int tmp_val = *(volatile unsigned int *)reg;

	tmp_val &= ~(0x1 << offset);
	(*(volatile unsigned int *)reg) = tmp_val;
}

void xreg_set_field(void *reg, unsigned int val, unsigned int bits, unsigned int offset)
{
	unsigned int tmp_val = *(volatile unsigned int *)reg;

	tmp_val &= ~(_xreg_mask[bits] << offset);
	tmp_val |= ((val & _xreg_mask[bits]) << offset);
	(*(volatile unsigned int *)reg) = tmp_val;
}

unsigned int xreg_get_field(void *reg, unsigned int bits, unsigned int offset)
{
	return (((*(volatile unsigned int *)reg) >> offset) & (_xreg_mask[bits]));
}
