#ifndef _PKA_H_
#define _PKA_H_

#include "config.h"

/* stage1 only need ECC-256 and RSA-2028 */
#define PKA_SIMPLE_FUNC

#ifdef PKA_SIMPLE_FUNC
typedef enum {
	PKA_256BIT_OPERAND_SIZE = 256,
	PKA_2048BIT_OPERAND_SIZE = 2048,
} PKA_OPERAND_SIZE;
#else
typedef enum {
	PKA_160BIT_OPERAND_SIZE = 160,
	PKA_192BIT_OPERAND_SIZE = 192, PKA_224BIT_OPERAND_SIZE = 224,
	PKA_256BIT_OPERAND_SIZE = 256,
	PKA_320BIT_OPERAND_SIZE = 320,
	PKA_384BIT_OPERAND_SIZE = 384,
	PKA_512BIT_OPERAND_SIZE = 512,
	PKA_768BIT_OPERAND_SIZE_= 768,
	PKA_1024BIT_OPERAND_SIZE = 1024,
	PKA_1536BIT_OPERAND_SIZE = 1536,
	PKA_2048BIT_OPERAND_SIZE = 2048,
	PKA_3072BIT_OPERAND_SIZE = 3072,
	PKA_4096BIT_OPERAND_SIZE = 4096,
} PKA_OPERAND_SIZE;
#endif

struct pka_ecc_pmult_param {
	const unsigned int *Px;
	const unsigned int *Py;
	const unsigned int *a;
	const unsigned int *k;
	const unsigned int *p;
};

struct pka_ecc_padd_param {
	const unsigned int *Px;
	const unsigned int *Py;
	const unsigned int *Qx;
	const unsigned int *Qy;
	const unsigned int *a;
	const unsigned int *p;
};

/*
 * PKA_MODADD
 *
 * DESCRIPTION
 *     result = x + y mod m provided (0 <= x < 2m) or (0 < x < 2ceil(log2(m))),
 *     whichever is less, and (0 <= y < 2m) or (0 < y < 2ceil(log2(m))),
 *     whichever is less
 *     m is modulus
 * */
int PKA_MODADD(unsigned int operand_size, const unsigned int *x, const unsigned int *y, const unsigned int *m, unsigned int *result);

/*
 * PKA_ECC_PMULT
 *
 * DESCRIPTION
 *    Q = kP provided Pxy is on the curve defined by a, b, and p, and
 *    (1 < k < order-of-base-point).
 *
 * */
int PKA_ECC_PMULT(unsigned int operand_size, struct pka_ecc_pmult_param *param, unsigned int *Qx, unsigned int *Qy);

/*
 * PKA_ECC_PADD
 *
 * DESCRIPTION
 *    R = P + Q provided Pxy and Qxy are on the curve defined by a, b, and p,
 *    and Px ≠ ±Qx
 * */
int PKA_ECC_PADD(unsigned int operand_size, struct pka_ecc_padd_param *param, unsigned int *Rx, unsigned int *Ry);

/*
 * PKA_Montgomery_Pre
 *
 * DESCRIPTION
 *    r_inv = 1 / r mod m provided m is of a size contained in the operand set
 *    size
 *    mp = m' = (r·r-1 – 1)/m provided m is of a size contained in the operand
 *    set size
 *    r_sqr = r2 mod m provided m is of a size contained in the operand set size
 *
 * */
int PKA_Montgomery_Pre(unsigned int operand_size, const unsigned int *m);

/*
 * PKA_MODEXP
 *
 * DESCRIPTION
 *    result = x^y mod m provided GCD(x,y,m) = 1, (0 < x < 2ceil(log2(m))), and
 *    (0 < y < 2ceil(log2(m)))
 *    m is modulus
 *
 * */
int PKA_MODEXP(unsigned int operand_size, const unsigned int *x, const unsigned int *m, unsigned int *result);
#endif
