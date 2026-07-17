#include <stdio.h>
#include "clp300_rom_fw.h"
#include "pka.h"
#include "config.h"

/* PKA Control & Status register */
#define CTRL				(*(volatile unsigned int*)(REG_BASE_PKA + 0x00))
#define ENTRY_PNT			(*(volatile unsigned int*)(REG_BASE_PKA + 0x04))
#define RTN_CODE			(*(volatile unsigned int*)(REG_BASE_PKA + 0x08))
#define STACK_PNTR			(*(volatile unsigned int*)(REG_BASE_PKA + 0x10))
#define STAT				(*(volatile unsigned int*)(REG_BASE_PKA + 0x20))
#define FLAGS				(*(volatile unsigned int*)(REG_BASE_PKA + 0x24))
#define WATCHDOG			(*(volatile unsigned int*)(REG_BASE_PKA + 0x28))
#define CYCLES_SINCE_GO			(*(volatile unsigned int*)(REG_BASE_PKA + 0x2C))
#define INDEX_I				(*(volatile unsigned int*)(REG_BASE_PKA + 0x30))
#define INDEX_J				(*(volatile unsigned int*)(REG_BASE_PKA + 0x34))
#define INDEX_K				(*(volatile unsigned int*)(REG_BASE_PKA + 0x38))
#define INDEX_L				(*(volatile unsigned int*)(REG_BASE_PKA + 0x3C))
#define IRQ_EN				(*(volatile unsigned int*)(REG_BASE_PKA + 0x40))
#define BANK_SW_A			(*(volatile unsigned int*)(REG_BASE_PKA + 0x50))
#define BANK_SW_B			(*(volatile unsigned int*)(REG_BASE_PKA + 0x54))
#define BANK_SW_C			(*(volatile unsigned int*)(REG_BASE_PKA + 0x58))
#define BANK_SW_D			(*(volatile unsigned int*)(REG_BASE_PKA + 0x5C))

//#define PKA_VERBOSE_DEBUG
#ifdef PKA_VERBOSE_DEBUG
#define PKA_DEBUG(format, ...) printf(format, ##__VA_ARGS__)
#else
#define PKA_DEBUG(format, ...)
#endif

enum CTRL_BASE_RADIX {
	BASE_RADIX_256_BIT  = 2,
	BASE_RADIX_512_BIT  = 3,
	BASE_RADIX_1024_BIT = 4,
	BASE_RADIX_2048_BIT = 5,
	BASE_RADIX_4096_BIT = 6
};

struct pka_data_regs {
	unsigned int A[8];
	unsigned int B[8];
	unsigned int C[8];
	unsigned int D[8];
};

#ifdef PKA_SIMPLE_FUNC
static unsigned int mp[PKA_2048BIT_OPERAND_SIZE / 32] = {-1};
static unsigned int r_sqr[PKA_2048BIT_OPERAND_SIZE / 32] = {-1};
#else
static unsigned int mp[PKA_4096BIT_OPERAND_SIZE / 32] = {-1};
static unsigned int r_sqr[PKA_4096BIT_OPERAND_SIZE / 32] = {-1};
#endif

static int pka_operand_size_is_support(unsigned int operand_size)
{
#ifdef PKA_SIMPLE_FUNC
	if(operand_size != PKA_256BIT_OPERAND_SIZE &&
	   operand_size != PKA_2048BIT_OPERAND_SIZE)
		return 0;
#else
	if (operand_size != PKA_160BIT_OPERAND_SIZE &&
	    operand_size != PKA_192BIT_OPERAND_SIZE &&
	    operand_size != PKA_224BIT_OPERAND_SIZE &&
	    operand_size != PKA_256BIT_OPERAND_SIZE &&
	    operand_size != PKA_320BIT_OPERAND_SIZE &&
	    operand_size != PKA_384BIT_OPERAND_SIZE &&
	    operand_size != PKA_512BIT_OPERAND_SIZE &&
	    operand_size != PKA_768BIT_OPERAND_SIZE_&&
	    operand_size != PKA_1024BIT_OPERAND_SIZE &&
	    operand_size != PKA_1536BIT_OPERAND_SIZE &&
	    operand_size != PKA_2048BIT_OPERAND_SIZE &&
	    operand_size != PKA_3072BIT_OPERAND_SIZE &&
	    operand_size != PKA_4096BIT_OPERAND_SIZE)
		return 0;
#endif
	return 1;
}

static unsigned int _pka_operand_size_align_reg(unsigned int operand_size)
{
	unsigned int operand_size_align;

#ifdef PKA_SIMPLE_FUNC
	if (operand_size <= PKA_256BIT_OPERAND_SIZE)
		operand_size_align = PKA_256BIT_OPERAND_SIZE;
	else if (operand_size <= PKA_2048BIT_OPERAND_SIZE)
		operand_size_align = PKA_2048BIT_OPERAND_SIZE;
	else
		operand_size_align = PKA_256BIT_OPERAND_SIZE;
#else
	if (operand_size <= PKA_256BIT_OPERAND_SIZE)
		operand_size_align = PKA_256BIT_OPERAND_SIZE;
	else if (operand_size <= PKA_512BIT_OPERAND_SIZE)
		operand_size_align = PKA_512BIT_OPERAND_SIZE;
	else if (operand_size <= PKA_1024BIT_OPERAND_SIZE)
		operand_size_align = PKA_1024BIT_OPERAND_SIZE;
	else if (operand_size <= PKA_2048BIT_OPERAND_SIZE)
		operand_size_align = PKA_2048BIT_OPERAND_SIZE;
	else if (operand_size <= PKA_4096BIT_OPERAND_SIZE)
		operand_size_align = PKA_4096BIT_OPERAND_SIZE;
	else
		operand_size_align = PKA_256BIT_OPERAND_SIZE;
#endif

	return operand_size_align;
}

static int _pka_get_base_radix(unsigned int operand_size)
{
	int base_radix;

#ifdef PKA_SIMPLE_FUNC
	if (operand_size <= PKA_256BIT_OPERAND_SIZE)
		base_radix = BASE_RADIX_256_BIT;
	else if (operand_size <= PKA_2048BIT_OPERAND_SIZE)
		base_radix = BASE_RADIX_2048_BIT;
	else
		base_radix = BASE_RADIX_256_BIT;
#else
	if (operand_size <= PKA_256BIT_OPERAND_SIZE)
		base_radix = BASE_RADIX_256_BIT;
	else if (operand_size <= PKA_512BIT_OPERAND_SIZE)
		base_radix = BASE_RADIX_512_BIT;
	else if (operand_size <= PKA_1024BIT_OPERAND_SIZE)
		base_radix = BASE_RADIX_1024_BIT;
	else if (operand_size <= PKA_2048BIT_OPERAND_SIZE)
		base_radix = BASE_RADIX_2048_BIT;
	else if (operand_size <= PKA_4096BIT_OPERAND_SIZE)
		base_radix = BASE_RADIX_4096_BIT;
	else
		base_radix = BASE_RADIX_256_BIT;
#endif

	return base_radix;
}

static int pka_get_data_regs_map(struct pka_data_regs *data_regs, unsigned int operand_size)
{
	int i = 0;
	unsigned int operand_size_in_bytes;

	if (!pka_operand_size_is_support(operand_size))
		return -1;

	operand_size_in_bytes = _pka_operand_size_align_reg(operand_size) >> 3;

	for (i = 0; i < 8; i++) {
		data_regs->A[i] = REG_BASE_PKA + 0x400 + i * operand_size_in_bytes;
		data_regs->B[i] = REG_BASE_PKA + 0x800 + i * operand_size_in_bytes;
		data_regs->C[i] = REG_BASE_PKA + 0xC00 + i * operand_size_in_bytes;
		data_regs->D[i] = REG_BASE_PKA + 0x1000 + i * operand_size_in_bytes;
	}

	return 0;
}

static int pka_go(unsigned int entry_pnt, unsigned int operand_size)
{
	unsigned int rt_code;
	unsigned int cyc;
	unsigned int base_radix;
	unsigned int partial_radix;
	unsigned int operand_size_align;

	operand_size_align = _pka_operand_size_align_reg(operand_size);
	if (operand_size == operand_size_align)
		partial_radix = 0;
	else if (operand_size < operand_size_align)
		partial_radix = (operand_size >> 5); //bit to word
	else
		return -2;
	base_radix = _pka_get_base_radix(operand_size);

	CTRL &= ~(0x7FF);
	CTRL |= (base_radix << 8) | partial_radix;
	ENTRY_PNT = entry_pnt;
	STACK_PNTR = 0;
	INDEX_I = 0;
	INDEX_J = 0;
	INDEX_K = 0;
	INDEX_L = 0;
	FLAGS = 0;
	WATCHDOG = 0xFFFFFFFF;
	CTRL |= 1 << 31;
	while((STAT & (1 << 30)) == 0);
	STAT = 1 << 30;
	cyc = CYCLES_SINCE_GO;
	PKA_DEBUG("cyc %d.\n",cyc);
	rt_code = (RTN_CODE >> 16) & 0xff;
	if(rt_code != 0)
		PKA_DEBUG("error: return code is %d.\n",rt_code);
	return rt_code;
}

static void pka_reg_cpy(unsigned int *dst, const unsigned int *src, unsigned int operand_size)
{
	unsigned int len = (operand_size >> 5); //bit to word
	while(len--)
		*dst++ = *src++;
}

int PKA_MODADD(unsigned int operand_size, const unsigned int *x, const unsigned int *y, const unsigned int *m, unsigned int *result)
{
	struct pka_data_regs data_regs;

	if (x == NULL || y == NULL || m == NULL || result == NULL)
		return -2;

	if (pka_get_data_regs_map(&data_regs, operand_size) != 0)
		return -2;

	pka_reg_cpy((unsigned int *)data_regs.A[0], x, operand_size);
	pka_reg_cpy((unsigned int *)data_regs.B[0], y, operand_size);
	pka_reg_cpy((unsigned int *)data_regs.D[0], m, operand_size);
	if (pka_go(ELP_CLUE_ENTRY_MODADD, operand_size) != 0)
		return -1;
	pka_reg_cpy(result, (const unsigned int *)data_regs.A[0], operand_size);

	return 0;
}

int PKA_Montgomery_Pre(unsigned int operand_size, const unsigned int *m)
{
	struct pka_data_regs data_regs;

	if (m == NULL)
		return -2;

	if (pka_get_data_regs_map(&data_regs, operand_size) != 0)
		return -2;
	// r_inv
	pka_reg_cpy((unsigned int *)data_regs.D[0], m, operand_size);
	if (pka_go(ELP_CLUE_ENTRY_CALC_R_INV, operand_size) != 0)
		return -1;
	// C0 is r_inv, D0 is modulus
	// mp
	pka_go(ELP_CLUE_ENTRY_CALC_MP, operand_size);
	// mp is D1
	pka_reg_cpy(mp, (const unsigned int *)data_regs.D[1], operand_size);
	pka_go(ELP_CLUE_ENTRY_CALC_R_SQR, operand_size);
	pka_reg_cpy(r_sqr, (const unsigned int *)data_regs.D[3], operand_size);

	return 0;
}


int PKA_ECC_PMULT(unsigned int operand_size, struct pka_ecc_pmult_param *param, unsigned int *Qx, unsigned int *Qy)
{
	struct pka_data_regs data_regs;

	if (param == NULL || param->Px == NULL || param->Py == NULL || param->a == NULL ||
	    param->k == NULL || param->p == NULL || Qx == NULL || Qy == NULL)
		return -2;

	if (pka_get_data_regs_map(&data_regs, operand_size) != 0)
		return -2;

	pka_reg_cpy((unsigned int *)data_regs.A[2], param->Px, operand_size);
	pka_reg_cpy((unsigned int *)data_regs.B[2], param->Py, operand_size);
	pka_reg_cpy((unsigned int *)data_regs.A[6], param->a, operand_size);
	pka_reg_cpy((unsigned int *)data_regs.D[7], param->k, operand_size);
	pka_reg_cpy((unsigned int *)data_regs.D[0], param->p, operand_size);
	pka_reg_cpy((unsigned int *)data_regs.D[1], mp, operand_size);
	pka_reg_cpy((unsigned int *)data_regs.D[3], r_sqr, operand_size);
	if (pka_go(ELP_CLUE_ENTRY_PMULT, operand_size) != 0)
		return -1;
	pka_reg_cpy(Qx, (const unsigned int *)data_regs.A[2], operand_size);
	pka_reg_cpy(Qy, (const unsigned int *)data_regs.B[2], operand_size);

	return 0;
}

int PKA_ECC_PADD(unsigned int operand_size, struct pka_ecc_padd_param *param, unsigned int *Rx, unsigned int *Ry)
{
	struct pka_data_regs data_regs;

	if (param == NULL || param->Px == NULL || param->Py == NULL ||
	    param->Qx == NULL || param->Qy == NULL || param->a == NULL ||
	    param->p == NULL || Rx == NULL || Ry == NULL)
		return -2;

	if (pka_get_data_regs_map(&data_regs, operand_size) != 0)
		return -2;

	pka_reg_cpy((unsigned int *)data_regs.A[2], param->Px, operand_size);
	pka_reg_cpy((unsigned int *)data_regs.B[2], param->Py, operand_size);
	pka_reg_cpy((unsigned int *)data_regs.A[3], param->Qx, operand_size);
	pka_reg_cpy((unsigned int *)data_regs.B[3], param->Qy, operand_size);
	pka_reg_cpy((unsigned int *)data_regs.A[6], param->a, operand_size);
	pka_reg_cpy((unsigned int *)data_regs.D[0], param->p, operand_size);
	pka_reg_cpy((unsigned int *)data_regs.D[1], mp, operand_size);
	pka_reg_cpy((unsigned int *)data_regs.D[3], r_sqr, operand_size);
	if (pka_go(ELP_CLUE_ENTRY_PADD, operand_size) != 0)
		return -1;
	pka_reg_cpy(Rx, (const unsigned int *)data_regs.A[2], operand_size);
	pka_reg_cpy(Ry, (const unsigned int *)data_regs.B[2], operand_size);

	return 0;
}

int PKA_MODEXP(unsigned int operand_size, const unsigned int *x, const unsigned int *m, unsigned int *result)
{
	struct pka_data_regs data_regs;
	unsigned int cnt;
	volatile unsigned int *ptr;

	if (x == NULL || m == NULL || result == NULL)
		return -2;

	if (pka_get_data_regs_map(&data_regs, operand_size) != 0)
		return -2;

	ptr = (volatile unsigned int *)data_regs.D[2];
	*ptr++ = 0x00010001;
	for(cnt = 0; cnt < 63; cnt++)
		*ptr++ = 0;

	pka_reg_cpy((unsigned int *)data_regs.A[0], x, operand_size);
	pka_reg_cpy((unsigned int *)data_regs.D[0], m, operand_size);
	pka_reg_cpy((unsigned int *)data_regs.D[1], mp, operand_size);
	pka_reg_cpy((unsigned int *)data_regs.D[3], r_sqr, operand_size);
	pka_go(ELP_CLUE_ENTRY_MODEXP, operand_size);
	pka_reg_cpy(result, (unsigned int *)data_regs.A[0], operand_size);

	return 0;
}
