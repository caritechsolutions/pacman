#ifndef __IRR_H__
#define __IRR_H__

unsigned char irr_init(void);
int irr_close(void);
int irr_read(void *buffer, uint32_t *len);
int irr_ioctl(uint32_t key,void *buf);

#endif

