.globl  hal_setjmp
//  .ent    hal_setjmp                                                                                                     
hal_setjmp:
    psrclr ie,fe
    mtcr r0,ss2               /*save the original stack point*/
    mov r0,r2                 /*now r0 point to the buf*/
    stm r1-r15,(r0)           /*r15 =pc before call */
                              /*not save pc */
    mfcr r0,ss2               /*get original stack point */
    stw  r0,(r2,60)           /*save r0*/
    movi r2,0
    psrset fe,ie
    rts
//  .end hal_setjmp

    .globl  hal_longjmp
//  .ent    hal_setjmp
hal_longjmp:
    psrclr ie,fe
    mtcr r2,ss2                /*save env*/
    mtcr r3,ss1
    mov  r0,r2                 /*r0 points to buf*/
    ldm r1-r15,(r0)            /*restor all the regs*/
    mfcr r2,ss2
    ldw  r0,(r2,60)            /*restore the stack*/
    mfcr r2,ss1
    cmpnei r2,0
    bt 1f
    movi r2,1
1:
    psrset fe,ie
    rts
    nop
//  .end hal_longjmp


