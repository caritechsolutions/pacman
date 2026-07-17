/* size_t strlen(const char *S)
 * entry: r0 -> string
 * exit: r0 = len
 */

		.section .text.strlen
                .global strlen
		.type strlen, %function
                .align 4

strlen:
        bic     r1, r0, $3              @ addr of word containing first byte
        ldr     r2, [r1], $4            @ get the first word
        ands    r3, r0, $3              @ how many bytes are duff?
        rsb     r0, r3, $0              @ get - that number into counter.
        beq     Laligned                @ skip into main check routine if no
                                        @ more
        orr     r2, r2, $0x000000ff     @ set this byte to non-zero
        subs    r3, r3, $1              @ any more to do?
        orrgt   r2, r2, $0x0000ff00     @ if so, set this byte
        subs    r3, r3, $1              @ more?
        orrgt   r2, r2, $0x00ff0000     @ then set.
Laligned:                               @ here, we have a word in r2.  Does it
        tst     r2, $0x000000ff         @ contain any zeroes?
        tstne   r2, $0x0000ff00         @
        tstne   r2, $0x00ff0000         @
        tstne   r2, $0xff000000         @
        addne   r0, r0, $4              @ if not, the string is 4 bytes longer
        ldrne   r2, [r1], $4            @ and we continue to the next word
        bne     Laligned                @
Llastword:                              @ drop through to here once we find a
        tst     r2, $0x000000ff         @
        addne   r0, r0, $1              @
        tstne   r2, $0x0000ff00         @ and add up to 3 bytes on to it
        addne   r0, r0, $1              @
        tstne   r2, $0x00ff0000         @ (if first three all non-zero, 4th
        addne   r0, r0, $1              @  must be zero)
        mov     pc,lr

