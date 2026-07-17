		.section .text.strncmp
                .global strncmp
		.type strncmp, %function
                .align 4

strncmp:
        /* if ((len - 1) < 0) return 0 */
        subs    r2, r2, #1
        movmi   r0, #0
        movmi   pc, lr

        /* ip == last src address to compare */
        add     ip, r0, r2
1:
        ldrb    r2, [r0], #1
        ldrb    r3, [r1], #1
        cmp     ip, r0
        cmpcs   r2, #1
        cmpcs   r2, r3
        beq     1b
        sub     r0, r2, r3
        mov     pc, lr

