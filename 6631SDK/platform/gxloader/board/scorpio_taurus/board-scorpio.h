
#ifndef __BOARD_SCORPIO_H__
#define __BOARD_SCORPIO_H__

struct scorpio_mulpin_config_s{
	unsigned short pin_id;
	unsigned char fun;
};

void scorpio_mulpin_init(void);
void scorpio_vout_init(void);

#endif
