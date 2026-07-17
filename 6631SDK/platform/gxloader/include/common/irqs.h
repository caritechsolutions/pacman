/*****************************************
  Copyright (c) 2002-2007
  Nationalchip Science & Technology Co., Ltd. All Rights Reserved
  Proprietary and Confidential
 *****************************************/

#ifndef __ASM_ARCH_IRQS_H
#define __ASM_ARCH_IRQS_H


/*
 *  IRQ
 */
#define IRQ_SOFTINT                     0	// Software interrupt
#define IRQ_UART0                       1	// UART 1
#define IRQ_UART1                       2	// UART 2
#define IRQ_TIMER0                      5	// Timer 0
#define IRQ_TIMER1                      6	// Timer 1
#define IRQ_TIMER2                      7	// Reserved for Timer 2
#define IRQ_GPIO0                       13	// GPIO Interrupt #0
#define IRQ_GPIO1                       14	// GPIO Interrupt #1
#define IRQ_GPIO2                       15	// GPIO Interrupt #2
#define IRQ_GPIO3                       16	// GPIO Interrupt #3
#define IRQ_PCILOCALBUSFAULT            20	// PCI Local Bus Fault
#define IRQ_EXTERNAL                    21	// Reserved for External interrupt


#define NR_IRQS                         64
#define NR_FIQS                         64

#endif
