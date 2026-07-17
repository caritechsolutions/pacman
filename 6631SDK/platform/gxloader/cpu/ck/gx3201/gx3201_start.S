/*****************************************
 Copyright (c) 2002-2003
 Nationalchip Science & Technology Co., Ltd. All Rights Reserved
 Proprietary and Confidential
 *****************************************/

/* This file is part of the Nationalchip boot loader */

#define __ASSEMBLY__

#include <config.h>

#define INTC_irq_MASK       (0x50)
#define INTC_irq_MASK_HI    (0x54)

    .text
    .align       0x2     # align on 2-byte range

    .global stage0_start                         # EXPORT entry for gdb
    .global ResetEntry                           # EXPORT entry for flash

#  NOTE: for the reason of bootrom, we MUST locate the exception vector table at the beginning,
#        and the 2nd long MUST be 300155aa.
ResetEntry:
    jsri        stage0_start
    jsri        stage0_start
    .long       __misaligned_access              #   1  0x004  Misaligned Access (The bootloader valid flag for nationalchip bootrom)
    .long       __access_error                   #   2  0x008  Access Error
    .long       __divide_by_zero                 #   3  0x00C  Divided by Zero
    .long       __illegal_instruction            #   4  0x010  Illegal Instruction
    .long       __privilege_violation            #   5  0x014  Privilege Violation
    .long       __trace_exception                #   6  0x018  Trace Exception
    .long       __breakpoint_exception           #   7  0x01C  Breakpoint Exception
    .long       __unrecoverable_error            #   8  0x020  Unrecoverable Error
    .long       __soft_reset                     #   9  0x024  Soft Reset
    .long       __normal_autovector_interrupt    #  10  0x028  Normal Autovector Interrupt
    .long       __fast_autovector_interrupt      #  11  0x02C  Fast Interrupt Autovector (AF used)
    .long       __hardware_accelerator           #  12  0x030  Hardware Accelerator
    .space      0x0C                             #  13-15      Unused exception space
    # NOTE: we don't define 128 vectors totally (512 bytes), to minimize the bootloader size

stage0_start:
	psrclr  ee,fe,ie
	psrclr  af
	lrw     r3, ResetEntry          # Initialize the Vector Base Register (VBR)
	mtcr    r3, vbr

#if (defined(CONFIG_ENABLE_LIB) && !defined(CONFIG_ENABLE_GDB_DEBUG))
	
	#--- Initialise stack register, stack_base is __bss_end_+1M
	lrw     r4, __bss_end__
	lrw     r5, 0x100000
	addu	r4, r5
	mov     r0, r4

	psrset  ee,fe,ie
	lrw     r4, main                # for ota
	jmp	r4
#else
	# flush v4 I/D caches
	# disable MMU stuff and caches
	mfcr    r4, cr17
	bseti   r4, 0
	bseti   r4, 1
	bseti   r4, 4
	mtcr    r4, cr17
	mfcr    r4, cr18
	bclri   r4, 0
	bclri   r4, 1
	bseti   r4, 2
	bclri   r4, 3
	bseti   r4, 6
	mtcr    r4, cr18

	# nop cycles for stability (not necessary)
	mov     r0, r0
	mov     r0, r0
	mov     r0, r0
	mov     r0, r0

	# mask all IRQs by setting all bits in the INTMR - default
	lrw     r1, (REG_INTC_BASE_NO_MMU + INTC_irq_MASK)
	lrw     r2, 0xffffffff
	st      r2, (r1)
	lrw     r1, (REG_INTC_BASE_NO_MMU + INTC_irq_MASK_HI)
	st      r2, (r1)

	# stack pointer (CK use r0 as sp)
	lrw     r4, STAGE1_STACK_TOP_ADDR_NOMMU
	mov     r0, r4

#ifndef CONFIG_ENABLE_GDB_DEBUG
	# initialize PLL
	jsri    gx_setup_pll_mini_controller
	# initialize DRAM
	jsri    gx_setup_sdram_controller
#endif

mmu_set:
	# invalid tlb before set
	bgeni	r2, 27
	cpseti	cp15
	cpwcr	r2, cpcr8

	# set MMU mapping table of address which starting from 0x00000000
	movi    r2, 0
	cpseti  cp15                    # set current coprocess is CP15
	cpwcr   r2, cpcr5               # CPCR5 for cp15, used when TLB Miss; PTE base will not be used, set to 0x0

	# to set page size
	lrw     r2, 0xff                # page size for 1MB/page
	lsli    r2, 13
	cpwcr   r2, cpcr6

	# to select MMU entry in jTLB
	lrw     r2, 0x0
	cpwcr   r2, cpcr1

	# to set Virtual start address
	lrw     r2, 0x0                 # Virtual address is set to 0x00000000
	cpwcr   r2, cpcr4               # MMU EntryHi/Bad VPN Register

	# to set page attribute and coresponding Physical addresses
	movi    r2, 0x1b                # cacheable, Dirty(readonly) and Valid
//	movi    r2, 0x17                # not cache rom.
	cpwcr   r2, cpcr2               # PFN = 0x00000000; C=011, Cacheable; D=1, Writeable; V=1, mapping Valid

	lrw     r2, 0x00100000
	lsri    r2, 6
//	movi    r3, 0x1f
	movi    r3, 0x17		# not cache sram
	or      r2, r3
	cpwcr   r2, cpcr3               # PFN = 0x00100000; C=011, Cacheable; D=1, Writeable; V=1, mapping Valid

	# write data to jTLB
	bseti   r2, 28
	cpwcr   r2, cpcr8
	movi    r2, 0x0
	cpwcr   r2, cpcr8

	mfcr    r4, cr18
	bclri   r4, 1
	bseti   r4, 0
	bseti   r4, 2
	bseti   r4, 3
	bseti   r4, 4
	bseti   r4, 5
	bseti   r4, 6
	bseti   r4, 7
	bclri   r4, 18                  # disable double cache line
#	bseti   r4, 18                  # enable double cache line
	mtcr    r4, cr18

#ifdef ENABLE_DENALI_ELF
	jsri	gx_setup_pll_full_controller
	lrw	r4, SRAMBASE
	lrw	r5, 0
	st	r5, (r4, 0)
	bkpt
#endif

#if (defined(CONFIG_ENABLE_MEMORY_TEST) || defined(CONFIG_ENABLE_MEMORY_PHASE_TEST))
	#memory test program shall not set stack pointer to address at dram
#else
	#--- Initialise stack register
	lrw     r4, STACK_TOP_ADDR
	mov     r0, r4
#endif

#ifndef CONFIG_ENABLE_GDB_DEBUG
#if defined(BOOT_FROM_FLASH)
	# copy bootloader into DRAM
	# loading priority: SPI -> NOR -> NAND
	jsri    gx_copy_bootloader
#endif
#endif

	#jump to C entry in the RAM
	psrset  ee,fe,ie

#ifdef CONFIG_ENABLE_LIB
	lrw     r4, main
#else
	lrw     r4, entry
#endif

	jmp     r4
#endif

.macro cpu_push_regs                # push_regs
	subi	r0,32
	subi	r0,32
	stm	r1-r15,(r0)
.endm

.macro cpu_pop_regs                 # pop_regs
	ldm	r1-r15,(r0)
	addi	r0,32
	addi	r0,32
.endm

#ifdef CONFIG_ENABLE_IRQ
# when interrupt occurs, we jump into SDRAM
__normal_autovector_interrupt:
	cpu_push_regs
	jbsr    gx_irq_handler
	cpu_pop_regs
	rte

__fast_autovector_interrupt:
	cpu_push_regs
	jbsr    gx_fiq_handler
	cpu_pop_regs
	rfi
#endif

# all other exception not handled now
__misaligned_access:
__access_error:
__divide_by_zero:
__illegal_instruction:
__privilege_violation:
__trace_exception:
__breakpoint_exception:
__unrecoverable_error:
__soft_reset:
#ifndef CONFIG_ENABLE_IRQ
__normal_autovector_interrupt:
__fast_autovector_interrupt:
#endif
__hardware_accelerator:
	br .

