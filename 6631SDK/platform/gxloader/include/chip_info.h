#ifndef __CHIP_INFO_H__
#define __CHIP_INFO_H__

#define GX_CHIP_ID_LEN 8

unsigned char *gx_get_chip_name(void);
unsigned char *gx_get_chip_id(void);
unsigned int gxcore_chip_probe(void);
unsigned int gxcore_chip_sub_probe(void);
unsigned int gxcore_chip_version(void);
int gxcore_chip_nagra_mode(void);
int gxcore_chip_tee_secure_enable(void);

#endif
