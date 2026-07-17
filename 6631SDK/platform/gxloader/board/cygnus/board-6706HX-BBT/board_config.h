#ifndef __PERIPH_CONFIG_H_
#define __PERIPH_CONFIG_H_

/* SPI FLASH */
#define CONFIG_FLASH_SPI_CLK_SRC          297000000    /* 297MHz */
#define CONFIG_SF_DEFAULT_CLKDIV          6            /* 分频数必须为偶数且非0 */
#define CONFIG_SF_DEFAULT_SAMPLE_DELAY    8
#define CONFIG_SF_DEFAULT_SPEED           (CONFIG_FLASH_SPI_CLK_SRC / CONFIG_SF_DEFAULT_CLKDIV)

#endif

