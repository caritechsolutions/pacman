
#include "sys/types.h"
#include "eeprom.h"
#include <stdio.h>
#include <string.h>

#define MAX_BUFFER_LEN     0x1000
#define MAX_PATH_LEN  64
#define RAND_TEST_ADDR_MAX   0x1000
#define RAND_TEST_LEN_MAX    0x1000
#define RAND_TEST_RANDMAX    0xfff

static unsigned short int w_buf[MAX_BUFFER_LEN];
static unsigned char r_buf[MAX_BUFFER_LEN];

typedef struct eprom_test {
	int addr;
	int len;
} eprom_test_t;

static eprom_test_t test_array[] = {
	{697,99},{2510,2933},{2638,1081},{1265,398},{216,3917},
	{0,4096},{31,328},{53,521},{67,753},{81,933},{99,1137},
	{115,283},{231,654},{451,267},{531,853},{956,364},
	{1126,3567},{2349,314},
};

static unsigned long long holdrand_test = 1; //随机数种子
static int rand_test(void)
{
	return (((holdrand_test = holdrand_test * 214013L + 2531011L) >> 16 ) & 0xfff);
}

static void write_random(int offset, unsigned char *buf, int len)
{
	eeprom_write(buf, offset, len);
}
static void read_random(int offset, unsigned char *buf, int len)
{
	eeprom_read(buf, offset, len);
}

/*Random address write and read test*/
static int random_write_read(int addr, int len)
{
	int i;
	int tmp_len = len;
	unsigned char *buf = (unsigned char *)w_buf;
	if(addr > MAX_BUFFER_LEN)
		return -1;
	if(addr + len > MAX_BUFFER_LEN)
	{
		tmp_len = MAX_BUFFER_LEN - addr;
	}

	memset(r_buf, '0', MAX_BUFFER_LEN);

	for(i = 0; i < (tmp_len + 1)/2; i++)
	{
		w_buf[i] = i;
	}

	write_random(addr, buf, tmp_len);
	read_random(addr, r_buf, tmp_len);

	for(i = 0; i < tmp_len; i++)
	{
		if(buf[i] != r_buf[i])
			break;
	}
	if(i == tmp_len)
		return 0;
	else
	{
		printf("\nr_buf: %d buf: %d,i: %d\n", r_buf[i], buf[i], i);
		return -1;
	}
}

int eeprom_comtest(void)
{
	int i, num;
	unsigned int addr_tmp, len_tmp;
	num = sizeof(test_array)/sizeof(eprom_test_t);

	printf("\ntotal test num: %d.\n", num);
	for(i = 0; i < num; i++)
	{
		printf("\nnum: %d.\n", i);
		if(random_write_read(test_array[i].addr, test_array[i].len))
		{
			printf("*****eeprom_test_failed:addr=0x%x, len = %d\n\n\n", \
				test_array[i].addr, test_array[i].len);
			while(1);
		}
		printf(">>>>>eeprom_test_success:addr=0x%x, len = %d\n\n\n", \
			test_array[i].addr, test_array[i].len);
	}
	while(1)
	{
		addr_tmp = (RAND_TEST_ADDR_MAX * rand_test()/(0xfff+1));
		len_tmp = (RAND_TEST_LEN_MAX * rand_test()/(0xfff+1));
		if(random_write_read(addr_tmp, len_tmp))
		{
			printf("*****eeprom_test_failed:addr=0x%x, len = %d\n\n\n", \
				addr_tmp, len_tmp);
			//continue;
			while(1);
		}
		printf(">>>>>eeprom_test_success:addr=0x%x, len = %d\n\n\n", \
			addr_tmp, len_tmp);
	}
	return 0;
}

