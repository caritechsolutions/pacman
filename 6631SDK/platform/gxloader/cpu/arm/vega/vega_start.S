#define __ASSEMBLY__
#include <config.h>

	.global ResetEntry
	.extern entry
#define CR_V	                     (1 << 13)	/* Vectors relocated to 0xffff0000	*/

ResetEntry:
	b	reset
	ldr pc, _undefined_instruction
	ldr pc, _software_interrupt
	ldr pc, _prefetch_abort
	ldr pc, _data_abort
	ldr pc, _not_used
	ldr pc, _irq
	ldr pc, _fiq

_undefined_instruction: .word undefined_instruction
_software_interrupt:    .word software_interrupt
_prefetch_abort:    .word prefetch_abort
_data_abort:        .word data_abort
_not_used:      .word not_used
_irq:           .word irq
_fiq:           .word fiq

clear_cntvoff:
	/* CNTVOFF Only accessible from Hyp mode, or from Monitor mode when SCR.NS is set to 1 */
	/* enter monitor mode */
	mrs r2, cpsr
	bic r2, r2, #0x1f
	orr r2, r2, #0x16           @ 0x16: monitor mode
	msr cpsr, r2

	/* set SCR.NS bit, copy from uboot */
	mrc p15, 0, r5, c1, c1, 0   @ read SCR
	bic r5, r5, #0x4a           @ clear IRQ, EA, nET bits
	orr r5, r5, #0x31           @ enable NS, AW, FW bits
	                            @ FIQ preserved for secure mode
	mcr p15, 0, r5, c1, c1, 0   @ write SCR (with NS bit set)
	isb

	/* clear CNTVOFF */
	movne   r4, #0
	mcrr p15, 4, r4, r4, c14    @ Reset CNTVOFF to zero
	isb

	/* clear SCR.NS for secure state */
	bic r5, r5, #0x01
	mcr p15, 0, r5, c1, c1, 0  @ write SCR (with NS bit set)
	isb

	/* back to svc mode */
	bic r2, r2, #0x1f
	orr r2, r2, #0x13          @ 0x13: svc mode
	msr cpsr, r2
	movs pc, lr

reset:
#if !defined(CONFIG_REE_LOADER)
	/* clear CNTOFF for arm generic timer */
	bl clear_cntvoff
#endif

	// halt cpu1/2/3
	mrc     p15, 0, r0, c0, c0, 5
	ubfx    r0, r0, #0, #2
	cmp     r0, #0
	wfine

	// Disable Watchdog
	ldr r0, =(REG_BASE_WDT - GX_REG_VIRTUAL_BASE1)
	mov r1, #0
	str r1, [r0]

	// Disable Interrupts (FIQ and IRQ), also set the cpu to SVC32 mode, except if in HYP mode already
	mrs r0, cpsr
	and r1, r0, #0x1f            // mask mode bits
	teq r1, #0x1a                // test for HYP mode
	bicne   r0, r0, #0x1f        // clear all mode bits
	orrne   r0, r0, #0x13        // set SVC mode
	orr r0, r0, #0xc0            // disable FIQ and IRQ
	msr cpsr,r0

	// Set V=0 in CP15 SCTLR register - for VBAR to point to vector
	mrc	p15, 0, r0, c1, c0, 0	 // Read CP15 SCTLR Register
	bic	r0, #CR_V		@ V = 0
	mcr	p15, 0, r0, c1, c0, 0	 // Write CP15 SCTLR Register

	// Set vector address in CP15 VBAR register
	ldr	r0, =ResetEntry
	mcr	p15, 0, r0, c12, c0, 0	 // Set VBAR

#if (defined(CONFIG_ENABLE_LIB) && !defined(CONFIG_ENABLE_GDB_DEBUG))
	#--- Initialise stack register, stack_base is __bss_end_+1M
	ldr     r0, =__bss_end__
	add     r0, r0, #0x100000
	mov     sp, r0
#if defined(CONFIG_ENABLE_IRQ)
	mrs r0, cpsr
	bic r0, r0, #0xc0            // enable FIQ and IRQ
	msr cpsr,r0
#endif
	#b main                # for ota
#else
	// Invalidate the I-Cache
	mov     r0, #0
	mcr     p15, 0, r0, c7, c5, 0
	// Invalidate the D-Cache
	mrc     p15, 1, r0, c0, c0, 0       // Read the Cache Size Identification register (CCSIDR)
	mov     r0, r0, LSL #10
	mov     r0, r0, LSR #23             // Mask off to leave the NumSets
	mov     r2, #0x0                    // Set r2 to initial MVA (Way=0, Set=0)
	mov     r1, #0                      // Use r1 as loop counter for WAYs
	mov     r3, #0x0                    // Use r3 as a loop counter for SETs

invalidate_cache_loop:
	mcr     p15, 0, r2, c7, c6, 2       // DCISW - Invalidate data cache by set/way
	add     r2, r2, #0x0020             // Increment the SET field
	add     r3, r3, #1                  // Increment loop counter
	cmp     r3, r0                      // Compare loop counter with num_sets
	ble     invalidate_cache_loop       // If (loop_counter =< num_sets) branch
										// Prepare register for next pass
	add     r2, r2, #0x40000000         // Increment WAY field
	and     r2, r2, #0xC0000000         // Clear the rest of the register (clear the SET field)
	mov     r3, #0                      // Reset loop counter

	add     r1, r1, #1
	cmp     r1, #4
	bne     invalidate_cache_loop

	mov		r4,#0xffffffff
	mcr		p15,0,r4,c3,c0,0        // domain

	// Invalidate TLB
	mov     r0, #0
	mcr     p15, 0, r0, c8, c7, 0   // Invalidate entire unified TLB
	mcr     p15, 0, r0, c8, c6, 0   // Invalidate entire data TLB
	mcr     p15, 0, r0, c8, c5, 0   // Invalidate entire instruction  TLB

	// set stage1 stack
	ldr	sp, =STAGE1_STACK_TOP_ADDR_NOMMU

mmu_set:
	// Set Table Base Control Register
	mov     r0, #0
	mcr     p15, 0, r0, c2, c0, 2

	// Page table modifications
	// Stored in memory is a set of page tables that flat-maps memory
	ldr     r0, =(HARDWARE_PAGE_TABLE_BASE - GX_REG_VIRTUAL_BASE1)        // Location of master copy of TTB
	// Set location of L1 page table
	mcr     p15, 0, r0, c2, c0, 0


	mrc p15, 0, r0, c1, c0, 1 // Read Auxiliary Control Register
	orr     r0, r0, #0x40               // enable SMP bit
#ifdef	ENABLE_SECURE_ALIGN
	orr     r0, r0, #0x0800             // Set L2RADIS 11bit
	orr     r0, r0, #0x1000             // Set L1RADIS 12bit
#endif
	mcr p15, 0, r0, c1, c0, 1 // Write Auxiliary Control Register

	// enable mmu
	dsb
	mrc     p15, 0, r0, c1, c0, 0       // Read SCTLR - System Control Register
	orr     r0, r0, #0x01               // Set M bit (bit 0)
	mcr     p15, 0, r0, c1, c0, 0       // Write SCTLR
	isb

	// enable cache
	mrc     p15, 0, r0, c1, c0, 0       // Read System Control Register configuration data
	orr     r0, r0, #0x0004             // Set C bit
	orr     r0, r0, #0x1000             // Set I bit
	mcr     p15, 0, r0, c1, c0, 0       // Write System Control Register configuration data

#ifdef ENABLE_DENALI_ELF
	bl    gx_setup_pll_full_controller
	b .
#endif

#ifdef ENABLE_BOOT_TOOL
	ldr	sp, =KERNEL_END_ADDR
#else
	ldr	sp, =STACK_TOP_ADDR
#endif

	#jump to C entry in the RAM
#if defined(CONFIG_ENABLE_IRQ)
	mrs r0, cpsr
	bic r0, r0, #0xc0            // enable FIQ and IRQ
	msr cpsr,r0
#endif

#ifdef CONFIG_ENABLE_LIB
	b main
#else
	b entry
#endif
#endif

/*
 * exception handlers
 */
undefined_instruction:
software_interrupt:
prefetch_abort:
data_abort:
not_used:
	b .

#ifndef CONFIG_ENABLE_IRQ
irq:
fiq:
	b .
#else
@
@ IRQ stack frame.
@
#define S_FRAME_SIZE	72

#define S_OLD_R0	68
#define S_PSR		64
#define S_PC		60
#define S_LR		56
#define S_SP		52

#define S_IP		48
#define S_FP		44
#define S_R10		40
#define S_R9		36
#define S_R8		32
#define S_R7		28
#define S_R6		24
#define S_R5		20
#define S_R4		16
#define S_R3		12
#define S_R2		8
#define S_R1		4
#define S_R0		0

#define MODE_SVC 0x13
#define I_BIT	 0x80

#define IRQ_STACK_START  (IRQ_STACK_TOP_ADDR - 4)
#define FIQ_STACK_START  (IRQ_STACK_START - CONFIG_STACKSIZE_IRQ)

/*
 * use bad_save_user_regs for abort/prefetch/undef/swi ...
 * use irq_save_user_regs / irq_restore_user_regs for IRQ/FIQ handling
 */
	.macro	irq_save_user_regs
	sub	sp, sp, #S_FRAME_SIZE
	stmia	sp, {r0 - r12}			@ Calling r0-r12
	@ !!!! R8 NEEDS to be saved !!!! a reserved stack spot would be good.
	add	r8, sp, #S_PC
	stmdb	r8, {sp, lr}^		@ Calling SP, LR
	str	lr, [r8, #0]		@ Save calling PC
	mrs	r6, spsr
	str	r6, [r8, #4]		@ Save CPSR
	str	r0, [r8, #8]		@ Save OLD_R0
	mov	r0, sp
	.endm

	.macro	irq_restore_user_regs
	ldmia	sp, {r0 - lr}^			@ Calling r0 - lr
	mov	r0, r0
	ldr	lr, [sp, #S_PC]			@ Get PC
	add	sp, sp, #S_FRAME_SIZE
	subs	pc, lr, #4		@ return & move spsr_svc into cpsr
	.endm

	.macro get_irq_stack			@ setup IRQ stack
	ldr	sp, =IRQ_STACK_START
	.endm

	.macro get_fiq_stack			@ setup FIQ stack
	ldr	sp, =FIQ_STACK_START
	.endm

/*
 * exception handlers
 */
	.align	5
irq:
	get_irq_stack
	irq_save_user_regs
	bl	gx_irq_handler
	irq_restore_user_regs

	.align	5
fiq:
	get_fiq_stack
	/* someone ought to write a more effiction fiq_save_user_regs */
	irq_save_user_regs
	bl	gx_fiq_handler
	irq_restore_user_regs
#endif
	.end

