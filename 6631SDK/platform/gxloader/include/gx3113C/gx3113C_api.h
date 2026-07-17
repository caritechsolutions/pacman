/*****************************************
  Copyright (c) 2002-2007
  Nationalchip Science & Technology Co., Ltd. All Rights Reserved
  Proprietary and Confidential
 *****************************************/

#ifndef __BOOTLOADER_GX3201API_H
#define __BOOTLOADER_GX3201API_H

//
// function prototypes
//

void arch_init(void);

// Clock
unsigned long gx_getclock(unsigned int);
unsigned int gx_getclockmhz(unsigned int);
void gx_setclockmhz(int clock);
char *gx_get_sdram_type(void);
unsigned int gx_get_sdram_freq(void);

// CPSR flag
unsigned int gx_saveflag_clif(void);
unsigned int gx_saveflag_clf(void);
unsigned int gx_saveflag_cli(void);
unsigned int gx_getflag(void);
void gx_restoreflag(unsigned int cpsr_reg);

// Cache
// clean : write dirty buffer (D cache only)
// invalidate : invalidate the contents of cache (I & D cache)
// flush : clean + invalidate
void gx_get_cache_state(int *picache, int *pdcache, int *pwriteback);
void gx_enable_cache(int icache, int dcache, int writeback);
void gx_clean_cache_data(void);
void gx_clean_cache_data_region(unsigned int from, unsigned int to);
void gx_invalidate_cache_instruction(void);
void gx_invalidate_cache_instruction_region(unsigned int from, unsigned int to);
void gx_invalidate_cache_data(void);
void gx_invalidate_cache_data_region(unsigned int from, unsigned int to);

void gx_flush_cache_all(void);
void gx_flush_cache_data(void);
void gx_flush_cache_data_region(unsigned int from, unsigned int to);


// GPIO
#define GPIO_INPUT  0
#define GPIO_OUTPUT 1

int gx_gpio_read(int gpio);
void gx_gpio_write(int gpio, int data);
void gx_gpio_setdirection(int gpio, int dir);

//signature
int signature_verification(void);

#endif
