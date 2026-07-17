#ifndef __SETJMP_H__
#define __SETJMP_H__

/*
 * 如下一些结构体的声明不标准，因此该文件不会作为 gxloader 中公共的头文件，而是作为当前小工程的
 * 头文件存在
 */
#if 0
typedef int __jmp_buf[6];

#define __BITS_PER_LONG 32

#define _NSIG 64
#define _NSIG_BPW __BITS_PER_LONG
#define _NSIG_WORDS (_NSIG / _NSIG_BPW)

typedef struct {
	unsigned long sig[_NSIG_WORDS];
} sigset_t;

typedef sigset_t __sigset_t;

struct __jmp_buf_tag {
	/* NOTE: The machine-dependent definitions of `__sigsetjmp'
	   assume that a `jmp_buf' begins with a `__jmp_buf' and that
	   `__mask_was_saved' follows it. Do not move these members
	   or add others before it. /
	   __jmp_buf __jmpbuf; / Calling environment. /
	   int __mask_was_saved; / Saved the signal mask? /
	   __sigset_t __saved_mask; / Saved signal mask. */
};

typedef struct __jmp_buf_tag jmp_buf[1];
#else
#define JMP_BUF_SIZE     16
typedef unsigned int gx_jmp_buf[ JMP_BUF_SIZE ];
typedef gx_jmp_buf jmp_buf;
#endif

extern int hal_setjmp(jmp_buf env);
#define setjmp( __env__ )  hal_setjmp( __env__ )
void longjmp(jmp_buf env, int val);

#endif
