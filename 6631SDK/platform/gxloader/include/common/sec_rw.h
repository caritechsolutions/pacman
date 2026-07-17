#ifndef __SEC_RW_H__
#define __SEC_RW_H__

void sec_write(unsigned int addr, unsigned int value, unsigned int delay);
unsigned int sec_read(unsigned int addr, unsigned int delay);

#endif

