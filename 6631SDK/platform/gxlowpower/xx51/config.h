#ifndef __CONFIG_H__
#define __CONFIG_H__

#define ENABLE_CEC
#if defined (SIRIUS_LOWPOWER)
#define MAX_GPIO_NUM    30
#define UART_TX_GPIO    1
#define IRR_GPIO        0
#define CEC_GPIO	2
// sirius-6613H1 public version board
#define GPIO_FP_DATA    18  //correspond CPU PORT31
#define GPIO_FP_CLK     19  //correspond CPU PORT32
#elif defined(TAURUS_LOWPOWER)
#define MAX_GPIO_NUM    6
#define UART_TX_GPIO    1
#define IRR_GPIO        0
#define CEC_GPIO	5
// taurus 6605H1 version board
#define GPIO_FP_DATA    2   //correspond CPU PORT17
#define GPIO_FP_CLK     3   //correspond CPU PORT18
#elif defined(GEMINI_LOWPOWER)
#define MAX_GPIO_NUM    15
#define UART_TX_GPIO    1
#define IRR_GPIO        0
#define CEC_GPIO	5
// gemini-6701 public version board
#define GPIO_FP_DATA    14   //correspond CPU PORT14
#define GPIO_FP_CLK     13   //correspond CPU PORT13
#elif defined(CYGNUS_LOWPOWER)
#define MAX_GPIO_NUM    9
#define UART_TX_GPIO    1
#define IRR_GPIO        2
#define CEC_GPIO	7
// cygnus 6701 public version board
#define GPIO_FP_DATA    1    //correspond CPU PORT18
#define GPIO_FP_CLK     0    //correspond CPU PORT17
#elif defined(CANOPUS_LOWPOWER)
#define MAX_GPIO_NUM    18
#define UART_TX_GPIO    9
#define IRR_GPIO        11
#define CEC_GPIO	16
//canopus 6631SH1A public version board
#define GPIO_FP_DATA    7    //correspond CPU PORT07
#define GPIO_FP_CLK     8    //correspond CPU PORT08
#elif defined(VEGA_LOWPOWER)
/* After the chip comes back, the following information must be modified */
#define MAX_GPIO_NUM    18
#define UART_TX_GPIO    9
#define IRR_GPIO        11
#define CEC_GPIO	16
//vega 6public version board
#define GPIO_FP_DATA    7    //correspond CPU PORT07
#define GPIO_FP_CLK     8    //correspond CPU PORT08
#elif defined(SCORPIO_LOWPOWER)
#define MAX_GPIO_NUM    6
#define UART_TX_GPIO    1
#define IRR_GPIO        0
#define CEC_GPIO	5
#define GPIO_FP_DATA    2   //correspond CPU PORT17
#define GPIO_FP_CLK     3   //correspond CPU PORT18
#elif defined(SCORPIO_TAURUS_LOWPOWER)
#define MAX_GPIO_NUM    6
#define UART_TX_GPIO    1
#define IRR_GPIO        0
#define CEC_GPIO	5
#define GPIO_FP_DATA    2   //correspond CPU PORT17
#define GPIO_FP_CLK     3   //correspond CPU PORT18
#endif


#endif
