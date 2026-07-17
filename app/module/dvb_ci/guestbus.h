#ifndef _GUESTBUS_H_
#define _GUESTBUS_H_


void gb_init(void);
int gb_detect_card(void);
void gb_reset_card(void);
void gb_readmem(unsigned int addr,unsigned char *rbuf,unsigned int len);
void gb_writemem(unsigned int addr, unsigned char *wbuf, unsigned int len);
void gb_readio(unsigned int addr,unsigned char *rbuf,unsigned int len);
void gb_writeio(unsigned int addr, unsigned char *wbuf, unsigned int len);



#endif
