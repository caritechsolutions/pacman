/*
 * Copyright (c) 2015, Linaro Limited
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include "linkage.h"

#ifdef CONFIG_REE_LOADER
.global ree_loader_sp
.global ree_loader_lr

.section .data
ree_loader_sp:
	.zero 4
ree_loader_lr:
	.zero 4
#endif

.section .text
	.macro SMCCC instr
	mov	r12, sp
	push	{r4-r7}
	ldm	r12, {r4-r7}
#ifdef CONFIG_REE_LOADER
	push    {r8-r9}
	ldr     r8, =ree_loader_sp
	ldr     r9, =ree_loader_lr
	str     sp, [r8]
	str     lr, [r9]
#endif
	smc     #0
#ifdef CONFIG_REE_LOADER
	pop	{r8-r9}
#endif
	pop	{r4-r7}
	ldr	r12, [sp, #(4 * 4)]
	stm	r12, {r0-r3}
	bx	lr
	.endm

/*
 * void smccc_smc(unsigned long a0, unsigned long a1, unsigned long a2,
 *		  unsigned long a3, unsigned long a4, unsigned long a5,
 *		  unsigned long a6, unsigned long a7, struct arm_smccc_res *res,
 *		  struct arm_smccc_quirk *quirk)
 */
ENTRY(__arm_smccc_smc)
	SMCCC SMCCC_SMC
ENDPROC(__arm_smccc_smc)

/*
 * void smccc_hvc(unsigned long a0, unsigned long a1, unsigned long a2,
 *		  unsigned long a3, unsigned long a4, unsigned long a5,
 *		  unsigned long a6, unsigned long a7, struct arm_smccc_res *res,
 *		  struct arm_smccc_quirk *quirk)
 */
ENTRY(__arm_smccc_hvc)
	SMCCC SMCCC_HVC
ENDPROC(__arm_smccc_hvc)
