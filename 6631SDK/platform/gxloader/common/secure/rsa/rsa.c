#include "rsa.h"
#include "stddef.h"
#include "sys/types.h"

static const unsigned char pkcs_sha256_id[19] = {
	0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
	0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05,
	0x00, 0x04, 0x20
};

static uint32_t rsa_n[66] = {-1};  // rsa_n is public key
static uint32_t rsa_x[66] = {-1};  // rsa_x is sign
static uint32_t omega = -1; // -N^(-1)
static uint32_t sqr_rho[66] = {-1};
static uint32_t tmp[66] = {-1};
static uint32_t one[66] = {-1};
static uint32_t ttt[66] = {-1};
static uint32_t xxx[66] = {-1};
static uint32_t rrr[66] = {-1};

unsigned char get_pkcs_header(int index)
{
	if(index == 0)
		return 0x00;
	else if(index == 1)
		return 0x01;
	else if(index >= 2 && index <= 203)
		return 0xff;
	else if(index == 204)
		return 0x00;
	else if(index >= 205)
		return (pkcs_sha256_id[index-205]);
}

static void *rsa_memcpy(void *dest, const void *src, int n)
{
	char *cdest = (char *)dest;
	const char *csrc = (const char *)src;
	int i = 0;

	for (i = 0; i < n; i++)
		cdest[i] = csrc[i];

	return dest;
}

static void big_reverse(uint8_t *buf, int len)
{
	int i = 0;
	char tmp = 0;

	for (i = 0; i < len / 2; i++) {
		tmp = buf[i];
		buf[i] = buf[len - i - 1];
		buf[len - i - 1] = tmp;
	}

}

static void big_mov(const uint32_t src[], uint32_t dst[])
{
	int i;
	for(i=0;i<66;i++)
	{
		dst[i] = src[i];
	}
}

static void big_shift_right(const uint32_t src[], uint32_t dst[])
{
	int i;
	for(i=0;i<65;i++)
		dst[i] = src[i+1];
	dst[65] = 0;
}
static void big_add(const uint32_t a[], const uint32_t b[], uint32_t sum[])
{
	int i;
	uint64_t sum64 = 0;
	uint32_t carry = 0;
	for(i=0;i<66;i++)
	{
		sum64 = (uint64_t)a[i] + (uint64_t)b[i] + (uint64_t)carry;
		sum[i] = sum64 & 0xffffffff;
		carry = sum64 >> 32;
	}
}

static void big_mul(const uint32_t big[], const uint32_t a32, uint32_t result[])
{
	int i;
	uint64_t p64 = 0;
	uint32_t carry = 0;
	for(i=0;i<66;i++)
	{
		p64 = (uint64_t)big[i] * (uint64_t)a32 + (uint64_t)carry;
		result[i] = p64 & 0xffffffff;
		carry = p64 >> 32;
	}
}

static void big_sub_if_gte(const uint32_t a[], const uint32_t n[], uint32_t result[])
{       // if a >= n, result = a - n, else result = a
	int i;
	uint64_t diff64 = 0;
	uint32_t borrow = 0;

	for(i=0;i<66;i++)
	{
		diff64 = (uint64_t)a[i] - (uint64_t)n[i] - (uint64_t)borrow ;
		tmp[i] = diff64  & 0xffffffff;
		borrow = (diff64 >> 32) ? 1 : 0;
	}
	if( borrow )
		big_mov(a,result);
	else
		big_mov(tmp,result);
}
static void calc_omega(void)
{
	int i;
	omega = 1;
	for(i=0;i<32;i++)
		omega = omega * omega * rsa_n[0];
	omega = 0-omega;
}

static void calc_sqr_rho(void)
{
	int i;
	sqr_rho[0] = 1;
	for(i=1;i<66;i++)
		sqr_rho[i] = 0;
	for(i=1;i<=2*64*32;i++)
	{
		big_add(sqr_rho,sqr_rho,sqr_rho);
		big_sub_if_gte(sqr_rho,rsa_n,sqr_rho);
	}
}

static void mont_mul(const uint32_t x[], const uint32_t y[], uint32_t result[])
{
	int i;
	uint32_t u;
	for(i=0;i<66;i++)
		rrr[i] = 0;
	for(i=0;i<64;i++)
	{
		u = (rrr[0] + y[i]*x[0])*omega;
		big_mul(x, y[i], tmp);
		big_add(rrr,tmp,rrr);
		big_mul(rsa_n,u,tmp);
		big_add(rrr,tmp,rrr);
		big_shift_right(rrr,rrr);
	}
	big_sub_if_gte(rrr,rsa_n,rrr);
	big_mov(rrr,result);
}

int rsa_dec(uint8_t *modulus, uint8_t *sign, uint8_t **hash)
{
	int i, ret;

	if (modulus == NULL || sign == NULL || hash == NULL)
		return -1;

	rsa_memcpy(rsa_n, modulus, 256);
	rsa_memcpy(rsa_x, sign, 256);
	big_reverse((uint8_t *)rsa_n, 256);
	big_reverse((uint8_t *)rsa_x, 256);

	calc_omega();
	calc_sqr_rho();

	one[0] = 1;
	for(i=1;i<66;i++)
		one[i] = 0;
	mont_mul(one,sqr_rho,ttt);
	mont_mul(rsa_x,sqr_rho,xxx);
	for(i=0;i<17;i++)
	{
		mont_mul(ttt,ttt,ttt);
		if(i==0 || i==16)
			mont_mul(ttt,xxx,ttt);
	}
	mont_mul(ttt,one,ttt);
	*hash = (uint8_t *)ttt;
	return 0;
}
