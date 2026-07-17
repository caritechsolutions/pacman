
#ifdef __thumb__
#define FUNC_START_ARM(_name_, _r_)              \
        .code   16                              ;\
        .thumb_func                             ;\
        .globl _name_                           ;\
_name_:                                         ;\
        ldr     _r_,=_name_ ## _ARM             ;\
        bx      _r_                             ;\
        .code   32                              ;\
_name_ ## _ARM:

#else

#define FUNC_START_ARM(_name_, _r_) \
        .globl _name_; \
_name_: 
        
#endif
FUNC_START_ARM(hal_setjmp, r2)
        stmea   r0,{r4-r14}
        mov     r0,#0
#ifdef __thumb__
        bx      lr
#else
        mov     pc,lr;          # return
#endif

//  hal_longjmp loads state from r0[arg0] and returns
        
FUNC_START_ARM(hal_longjmp, r2)
        ldmfd   r0,{r4-r14}
        mov     r0,r1;          # return [arg1]
#ifdef __thumb__
        bx      lr
#else
        mov     pc,lr
#endif

