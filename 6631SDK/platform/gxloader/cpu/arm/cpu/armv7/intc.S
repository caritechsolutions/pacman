#include "linkage.h"

/*
 *	turn on irq
 *
 */
ENTRY(enable_irq)
	mrs r0, cpsr
	bic r0, r0,  #0x80            // enable IRQ
	msr cpsr,r0
	bx lr
ENDPROC(enable_irq)

/*
 *	turn on fiq
 *
 */
ENTRY(enable_fiq)
	mrs r0, cpsr
	bic r0, r0,  #0x40            // enable FIQ
	msr cpsr,r0
	bx lr
ENDPROC(enable_fiq)

/*
 *	turn off irq
 *
 */
ENTRY(disable_irq)
	mrs r0, cpsr
	orr r0, r0,  #0x80            // disable IRQ
	msr cpsr,r0
	bx lr
ENDPROC(disable_irq)

/*
 *	turn off fiq
 *
 */
ENTRY(disable_fiq)
	mrs r0, cpsr
	orr r0, r0,  #0x40            // disable FIQ
	msr cpsr,r0
	bx lr
ENDPROC(disable_fiq)

