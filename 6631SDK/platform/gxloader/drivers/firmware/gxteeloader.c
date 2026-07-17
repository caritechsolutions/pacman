#include <stdio.h>
#include <gxteeloader_smccc.h>

#define TEELOADER_IS_INIT()                          \
	if (teeloader_init == 0) {                   \
		printf("gxteeloader has not init\n");\
		return -1;                           \
	}                                            \

static int teeloader_init = 0;

int gxteeloader_init(void)
{
	gxteeloader_args_t args = {0};
	gxteeloader_res_t res = {0};
	int error;

	if (teeloader_init > 0) {
		printf("gxteeloader has init\n");
		return 0;
	}
	teeloader_init = 1;

	args.a0 = GXTEELOADER_SMC(GXTEELOADER_INIT);
	invoke_gxteeloader_fn(&args, &res);

	error = res.a0;
	if (error < 0) {
		printf("tee loader init failed\n");
		return -1;
	}

	return error;
}

int gxteeloader_setup_pll(void)
{
	gxteeloader_args_t args;
	gxteeloader_res_t res = {0};
	int error;

	TEELOADER_IS_INIT();
	args.a0 = GXTEELOADER_SMC(GXTEELOADER_SETUP_PLL);
	invoke_gxteeloader_fn(&args, &res);

	error = res.a0;
	if (error < 0) {
		printf("tee loader setup pll failed\n");
		return -1;
	}

	return error;
}

int gxteeloader_power_reduction(void)
{
	gxteeloader_args_t args;
	gxteeloader_res_t res = {0};
	int error;

	TEELOADER_IS_INIT();
	args.a0 = GXTEELOADER_SMC(GXTEELOADER_SETUP_POWER_REDUCTION);
	invoke_gxteeloader_fn(&args, &res);

	error = res.a0;
	if (error < 0) {
		printf("tee loader setup power reduction failed\n");
		return -1;
	}

	return error;
}

int gxteeloader_load_teeos(void *teeos_img, unsigned long teeos_img_size)
{
	gxteeloader_args_t args;
	gxteeloader_res_t res = {0};
	int error;

	/* TEE_Loader map all memory to secure memory, can't access non-secure memory cache, so DTE_Boot_Code(REE) must
	 * clean non-secure memory cache to DRAM.
	 * */
	dcache_flush();

	TEELOADER_IS_INIT();
	args.a0 = GXTEELOADER_SMC(GXTEELOADER_LOAD_TEEOS);
	args.a1 = (unsigned long)teeos_img;
	args.a2 = teeos_img_size;
	invoke_gxteeloader_fn(&args, &res);

	error = res.a0;
	if (error < 0) {
		printf("tee loader decrypt and verify TEEOS failed\n");
		return -1;
	}

	return error;
}

int gxteeloader_jump_teeos(void)
{
	extern void return_from_tee(void);
	gxteeloader_args_t args;
	gxteeloader_res_t res = {0};
	int error;

	TEELOADER_IS_INIT();
	args.a0 = GXTEELOADER_SMC(GXTEELOADER_JUMP_TEEOS);
	args.a1 = (unsigned int)return_from_tee;
	invoke_gxteeloader_fn(&args, &res);

	/*
	 * After TEEOS completes initialization, the CPU is switched to REE through the SMC
	 * instruction and return_from_tee is executed. return_from_tee restores
	 * the context saved by invoke_gxteeloader_fn and returns here.
	 * */
	error = res.a0;
	if (error < 0) {
		printf("tee loader jump teeos failed\n");
		return -1;
	}

	return error;
}
