#ifndef __GXTEELOADER_SMCCC_H__
#define __GXTEELOADER_SMCCC_H__

#include <linux/arm-smccc.h>

typedef enum {
	GXTEELOADER_INIT,
	GXTEELOADER_SETUP_PLL,
	GXTEELOADER_SETUP_POWER_REDUCTION,
	GXTEELOADER_LOAD_TEEOS,
	GXTEELOADER_JUMP_TEEOS,
} GXTEELOADER_FUNC_NUM;

typedef struct {
	unsigned long a0;
	unsigned long a1;
	unsigned long a2;
	unsigned long a3;
	unsigned long a4;
	unsigned long a5;
	unsigned long a6;
	unsigned long a7;
} gxteeloader_args_t;

typedef struct arm_smccc_res gxteeloader_res_t;

#define GXTEELOADER_SMC(func_num)                                   \
	ARM_SMCCC_CALL_VAL(ARM_SMCCC_FAST_CALL, ARM_SMCCC_SMC_32,    \
			   ARM_SMCCC_OWNER_STANDARD, (func_num))

static inline void invoke_gxteeloader_fn(gxteeloader_args_t *args, gxteeloader_res_t *res)
{
	arm_smccc_smc(args->a0, args->a1, args->a2, args->a3, args->a4, args->a5, args->a6, \
		      args->a7, (struct arm_smccc_res *)res);
}


int gxteeloader_init(void);
int gxteeloader_setup_pll(void);
int gxteeloader_power_reduction(void);
int gxteeloader_load_teeos(void *teeos_img, unsigned long teeos_img_size);
int gxteeloader_jump_teeos(void);

#endif
