#define __ASSEMBLY__
#include <config.h>

	.extern decompress_stage2
	.global decompress_entry
decompress_entry:
#ifdef CONFIG_ARCH_ARM
	.rept   8
	mov     r0, r0
	.endr
	b 1f
	.word   _magic_sig      @ Magic numbers to help the loader
	.word   _magic_start    @ absolute load/run zImage address
	.word   _magic_end      @ zImage end address
	.word   0x04030201      @ endianness flag
1:
	push {r4, lr}
	bl decompress_stage2
	pop  {r4, pc}
#else
# The csky instruction is only 2 bytes long
	.rept   17
	mov     r0, r0
	.endr
	br 1f
	.word   _magic_sig      # Magic numbers to help the loader
	.word   _magic_start    # absolute load/run zImage address
	.word   _magic_end      # zImage end address
	.word   0x04030201      # endianness flag
1:
	subi	r0, r0, 8
	st	r15, (r0, 0)
	jsri decompress_stage2
	ld	r15, (r0, 0)
	addi	r0, r0, 8
	jmp	r15

#endif
