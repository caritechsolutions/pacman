
#define __INCLUDE_BY_ASM
#include "config.h"

#if ( GXCHIP_TYPE == GXCHIP_TYPE_GX3113C || \
     	GXCHIP_TYPE == GXCHIP_TYPE_GX3201 || \
     	GXCHIP_TYPE == GXCHIP_TYPE_GX3211 || \
     	GXCHIP_TYPE == GXCHIP_TYPE_GX6605S)
//**********************************************************************//
// Start for ck
//**********************************************************************//

.extern Standby
//.extern gWakeTime

.text
.global lopower_entry
.global __reset

//**********************************************************************//
// Reset Vector 0 "start" point
//**********************************************************************//

.align 4
__reset:
    jsri        lopower_entry
    jsri        lopower_entry
    .space     0x024
    .long       __normal_autovector_interrupt    #  10  0x028  Normal Autovector Interrupt

lopower_entry:
    psrclr	ee,ie,fe
    psrclr  af
    lrw     r3, __reset          # Initialize the Vector Base Register (VBR)
    mtcr    r3, vbr

	movi r8, 0x32
	mtcr r7, cr17
	movi r7, 0x20
	mtcr r7, cr18

	lrw   r8, __stack
	mov   r0, r8

    # mask all IRQs by setting all bits in the INTMR - default
    lrw     r1, (0x00500000 + 0x50)
    lrw     r2, 0xffffffff
    st      r2, (r1)
    lrw     r1, (0x00500000 + 0x54)
    st      r2, (r1)

    psrset  ee,fe,ie

	# Zero the bss region.
	lrw     r8, _sbss       	# Get start of bss
	lrw     r9, _ebss         	# Get end of bss
	cmphs	r8, r9			# if r8 >= r9, set C = 1
	bt	2f
	subu    r9, r8                  # Calculate size of bss
	lsri    r9, 2                   # Size of whole words
	movi    r10, 0                  # Set zero value to write
1:
	stw     r10, (r8)               # Zero next word
	addi    r8, 4                   # Increase bss pointer
	decne   r9                      # Decrease counter
	bt      1b                      # Repeat for all bss
2:

	jbsr	Standby

.macro cpu_push_regs                # push_regs
    subi    r0,32
    subi    r0,32
    stm r1-r15,(r0)
.endm

.macro cpu_pop_regs                 # pop_regs
    ldm r1-r15,(r0)
    addi    r0,32
    addi    r0,32
.endm

__normal_autovector_interrupt:
    cpu_push_regs
    jbsr    gx_irq_handler
    cpu_pop_regs
    rte

#else

//**********************************************************************//
// Start for arm
//**********************************************************************//

.extern Standby
//.extern gWakeTime
.global __reset

.text


.align 4
__reset:

	mrs	  r7,cpsr
	orr	  r7,r7,#((1 << 7) | (1 << 6))
	msr	  cpsr,r7

	ldr	  r8,=__stack
	mov	  SP,r8

	bl	  Standby
exit:
	bl	  .
	
#endif
