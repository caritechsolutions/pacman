#ifndef __BOARD_TAURUS_H__
#define __BOARD_TAURUS_H__

struct taurus_mulpin_config_s{
	unsigned char side;
	unsigned short chip_core_num;
	unsigned char sel0;
	unsigned char sel1;
	unsigned char sel2;
	unsigned char fun;
};

void taurus_mulpin_init(void);
void taurus_vout_init(void);
#endif
