#include "linkage.h"
#ifdef CONFIG_SYS_THUMB_BUILD
#define ARM(x...)
#define THUMB(x...)	x
#else
#define ARM(x...)	x
#define THUMB(x...)
#endif
#include "gxhwlib_registers.h"

ENTRY(__func_set_ree_mmu_cache)
    mrc p15, 0, r0, c3, c0, 0

    bic r0,r0,#0xc
    orr r0,r0,#4

    mcr p15, 0, r0, c3, c0, 0

    mov     r0, #0
    mcr     p15, 0, r0, c2, c0, 2

    ldr     r0, =(HARDWARE_PAGE_TABLE_BASE - GX_REG_VIRTUAL_BASE1)

    mcr     p15, 0, r0, c2, c0, 0

    mov		r4,#0xffffffff
    mcr		p15,0,r4,c3,c0,0        // domain

    mrc p15, 0, r0, c1, c0, 1 // Read Auxiliary Control Register
    orr     r0, r0, #0x40               // enable SMP bit
#ifdef  ENABLE_SECURE_ALIGN
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
   dsb
   isb
   bx lr
ENDPROC(__func_set_ree_mmu_cache)

ENTRY(hal_set_ree_mmu_cache)
 ARM(	stmfd	sp!, {r4-r5, r7, r9-r11, lr}	)
 THUMB(	stmfd	sp!, {r4-r7, r9-r11, lr}	)
	bl	__func_set_ree_mmu_cache
 ARM(	ldmfd	sp!, {r4-r5, r7, r9-r11, lr}	)
 THUMB(	ldmfd	sp!, {r4-r7, r9-r11, lr}	)
	bx	lr
ENDPROC(hal_set_ree_mmu_cache)

#ifdef CONFIG_REE_LOADER
/* ree_loader_sp and ree_loader_lr is defined cpu/arm/cpu/armv7/smccc-call.S */
.extern ree_loader_sp
.extern ree_loader_lr

ENTRY(return_from_tee)
/* Return from teeos, sp and lr are broken, ree_loader_sp and ree_loader_lr were saved in
 * cpu/arm/cpu/armv7/smccc-call.S before jump teeos.
 */
	ldr     r8, =ree_loader_sp
	ldr     r9, =ree_loader_lr
	ldr     sp, [r8]
	ldr     lr, [r9]
	pop	{r8-r9}
	pop	{r4-r7}
	ldr	r12, [sp, #(4 * 4)]
	stm	r12, {r0-r3}
#if defined(CONFIG_ENABLE_CMD) && CONFIG_BOOTDELAY > 0
	bl hal_set_ree_mmu_cache
	bl bootmenu
#else
	bx	lr
#endif
ENDPROC(return_from_tee)
#else
ENTRY(return_from_tee)
	ldr	sp, =STACK_TOP_ADDR
	bl get_cmdline
	sub r0, r0, #8
	mov r11, r0
	bl hal_set_ree_mmu_cache
	bl bootmenu
ENDPROC(return_from_tee)
#endif
