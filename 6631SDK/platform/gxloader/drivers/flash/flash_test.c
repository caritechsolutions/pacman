#include "stdio.h"
#include "io.h"
#include "util.h"
#include "config.h"
#include "gx_api.h"
#include "gxhwlib_registers.h"
#include <gx_flash_common.h>
#include "flash.h"
#include "string.h"
#include <gx_flash.h>
#include <gx_firewall.h>
#include "spi.h"

#ifdef CONFIG_ENABLE_FLASH_TEST

#define RAND_TEST_ADDR_MAX     0x100000
#define RAND_TEST_LEN_MAX      0x1000000
#define RAND_TEST_RANDMAX      0x7fff

static unsigned char sample_delay_test = 0; // 用于测试 Sample_delay，默认不开启

static uint64_t holdrand_test = 1;
static uint32_t flash_random_seed = 1;
void rand_init(uint32_t seed)
{
	flash_random_seed = seed;
	printf("random seed %d\n", (uint32_t)flash_random_seed);
}

uint32_t rand_test(void)
{
	return (((holdrand_test = holdrand_test * 214013L + 2531011L) >> 16 ) & 0x7fff);
}

void flash_ram_set(uint32_t addr, uint8_t *data, uint32_t len)
{
	uint8_t *ramaddr = (unsigned char *)addr;
	int i;

	for (i = 0; i < len; i++) {
		ramaddr[i] = data[i];
	}
}

void flash_ram_get(uint32_t addr, uint8_t *data, uint32_t len)
{
	uint8_t *ramaddr = (unsigned char *)addr;
	int i;

	for (i = 0; i < len; i++) {
		data[i] = ramaddr[i];
	}
}

#ifdef CONFIG_ENABLE_FIREWALL
void flash_firewall_test(unsigned int ram_addr, unsigned int ram_size, int read_permission, int write_permission)
{
	uint32_t erase_start, erase_end;
	uint8_t *ram_data;
	uint8_t *p, *p2;
	int i;

	ram_data = malloc(ram_size);
	if(ram_data == NULL) {
		printf("LINE%d:malloc failed.malloc len=0x%x\n", __LINE__, ram_size);
		while(1);
	}

	memset(ram_data, 0, ram_size);

	for (i = 0; i < ram_size; i++) {
		ram_data[i] = 0x5a;
	}

	if ((read_permission != 0) || (write_permission != 0)) {
		if ((read_permission != 0) && (write_permission != 0)) {
			printf("\n**************** Test firewall read and write permission ****************\n");
#if defined(CONFIG_ARCH_ARM_CANOPUS)
			firewall_config_filter(ram_addr, ram_size, MASTER_A7 | MASTER_SPI | MASTER_MAC_USB_NFC, MASTER_A7 | MASTER_SPI | MASTER_MAC_USB_NFC, 0);
#elif defined(CONFIG_ARCH_ARM_VEGA)
			firewall_config_filter(ram_addr, ram_size, (1 << 31) | (1 << 5), (1 << 31) | (1 << 5), 0);
#endif
		} else if (read_permission != 0) {
			printf("\n**************** Test firewall read permission ****************\n");
#if defined(CONFIG_ARCH_ARM_CANOPUS)
			firewall_config_filter(ram_addr, ram_size, MASTER_A7 | MASTER_SPI | MASTER_MAC_USB_NFC, MASTER_A7, 0);
#elif defined(CONFIG_ARCH_ARM_VEGA)
			firewall_config_filter(ram_addr, ram_size, (1 << 31) | (1 << 5), (1 << 31), 0);
#endif
		} else if (write_permission != 0) {
#if defined(CONFIG_ARCH_ARM_CANOPUS)
			firewall_config_filter(ram_addr, ram_size, MASTER_A7, MASTER_A7 | MASTER_SPI | MASTER_MAC_USB_NFC, 0);
#elif defined(CONFIG_ARCH_ARM_VEGA)
			firewall_config_filter(ram_addr, ram_size, (1 << 31), (1 << 31) | (1 << 5), 0);
#endif
			printf("\n**************** Test firewall write permission ****************\n");
		}
	} else {
#if defined(CONFIG_ARCH_ARM_CANOPUS)
		firewall_config_filter(ram_addr, ram_size, MASTER_A7, MASTER_A7, 0);
#elif defined(CONFIG_ARCH_ARM_VEGA)
		firewall_config_filter(ram_addr, ram_size, (1 << 31), (1 << 31), 0);
#endif
		printf("\n**************** Test firewall no read and write permission ****************\n");
	}

	printf("\n**************** SPI read test ****************\n");

	flash_ram_set(ram_addr, ram_data, ram_size);

	gxflash_erasedata(0, ram_size);

	gxflash_pageprogram(0, (unsigned char *)ram_addr, ram_size);

	p = malloc(ram_size);
	if(p == NULL)
	{
		printf("LINE%d:malloc failed.malloc len=0x%x\n", __LINE__, ram_size);
		while(1);
	}
	memset(p, 0, ram_size);
	gxflash_readdata(0, p, ram_size);
	for (i = 0; i < ram_size; i++) {
		if ((p[i]) != (ram_data[i])) {
			printf("\ncmp error.flash_data[%d]:0x%x, ram_data[%d]:0x%x\n", i, p[i], i, ram_data[i]);
			printf("\n\t >>> The current module does not have read permission.\n\n");
			break;
		}
	}

	if (i == ram_size)
		printf("\n\t >>> The current module read permission test succeed.\n\n");

	printf("\n**************** SPI write test ****************\n");

	gxflash_erasedata(0, ram_size);

	p2 = malloc(ram_size);
	if(p2 == NULL)
	{
		printf("LINE%d:malloc failed.malloc len=0x%x\n", __LINE__, ram_size);
		while(1);
	}
	memset(p2, 0, ram_size);
	flash_ram_set(ram_addr, p2, ram_size);

	for (i = 0; i < ram_size; i++) {
		p2[i] = 0xa5;
	}
	gxflash_pageprogram(0, p2, ram_size);

	gxflash_readdata(0, (unsigned char *)ram_addr, ram_size);

	memset(p, 0, ram_size);
	flash_ram_get(ram_addr, p, ram_size);

	for (i = 0; i < ram_size; i++) {
		if ((p2[i]) != (p[i])) {
			printf("\ncmp error.read from ddr data[%d]:0x%x, flash write to ddr data[%d]:0x%x\n", i, p[i], i, p2[i]);
			printf("\n\t >>> The current module does not have write permission.\n\n");
			break;
		}
	}

	if (i == ram_size)
		printf("\n\t >>> The current module write permission test succeed.\n\n");

	free(ram_data);
	free(p);
	free(p2);
}
#endif

//sflash_db_t sflash_test_db = {.param = 0};//DUAL_READ};
int sflash_test_single(uint32_t addr, uint32_t len)
{
	uint8_t *p, *p2, *p_src, *p_dst, *p_tmp;
	int i;
	uint32_t erase_start, erase_end;
	uint32_t block_size;

	block_size = gxflash_get_info(GX_FLASH_ERASE_SIZE);
	p = malloc(len);
	if(p == NULL)
	{
		printf("LINE%d:malloc failed.malloc len=0x%x\n", __LINE__, len);
		while(1);
	}

	gxflash_readdata(addr, p, len);
	printf("sflash_gx_readl finish.\n");
	erase_start = addr & ~(block_size - 1);
	erase_end = (addr + len + block_size - 1) / block_size * block_size;
	gxflash_erasedata(erase_start, erase_end - erase_start);
	printf("sflash erase finish.\n");

	printf("read addr=0x%x, len=0x%x\n", addr, len);
	gxflash_readdata(addr, p, len);
	printf("sflash_gx_readl finish.\n");
	for(i = 0, p_tmp = p; i < len; i++)
	{
		//printf("0x%x ", *p_tmp++);
		if(*p_tmp++ != 0xff)	/* 确认擦除后数据是否为全0xFF */
		{
			printf("erase error, i = %d, p[i]=0x%x \n", i, p[i]);
			if (sample_delay_test == 0) {
				//break;
				while(1);		//擦除错误则停止，便于打印观看
			} else {
				free(p);
				return -1;
			}
		}
	}
	/*printf("\nprintf finish.\n");*/


	p2 = malloc(len);
	if(p2 == NULL)
	{
		printf("LINE%d:malloc failed.malloc len=0x%x\n", __LINE__, len);
		while(1);
	}

	for (i = 0; i < len; i++) {
		p2[i] = rand_test();
	}
	//sflash_gx_pageprogram(&sflash_test_db, addr, p2, len);
	gxflash_pageprogram(addr, p2, len);
	printf("sflash page program finish.\n");
	//sflash_gx_readl(&sflash_test_db, addr, p, len);
	memset(p, 0, len);
	gxflash_readdata(addr, p, len);
	printf("sflash_gx_readl finish.\n");
	/*for(i = 0, p_tmp = p; i < len; i++)
	{
		printf("0x%x ", *p_tmp++);
	}
	printf("\nprintf finish.\n");*/
	p_src = p;
	p_dst = p2;
	for(i = 0; i < len; i++)
	{
		if((*p_src++) != (*p_dst++))
		{
			printf("cmp error. i = %d. read=0x%x, orig=0x%x\n", i, *(p_src-1), *(p_dst-1));
			free(p);
			free(p2);
			return -1;
		}
	}
	printf("cmp finish. src equal dst.\n");
	free(p);
	free(p2);
	return 0;
}

//sflash_db_t sflash_test_db = {.param = 0};//DUAL_READ};
int flash_nand_test_single(uint32_t addr, uint32_t len, uint32_t oob_addr, uint32_t oob_len, uint32_t mode)
{
	uint8_t *p=NULL, *p1=NULL, *p2=NULL, *p3=NULL, *p_src, *p_dst, *p_tmp;
	int i;
	uint32_t erase_start, erase_end;
	uint32_t block_size;

	if ((len == 0) && (oob_len == 0))
		return -1;

	block_size = gxflash_get_info(GX_FLASH_ERASE_SIZE);
	if(len > 0) {
		p = malloc(len);
		if(p == NULL)
		{
			printf("LINE%d:malloc failed.malloc len=0x%x\n", __LINE__, len);
			while(1);
		}
	}
	if(oob_len > 0) {
		p1 = malloc(oob_len);
		if(p1 == NULL)
		{
			printf("LINE%d:malloc failed.malloc ooblen=0x%x\n", __LINE__, oob_len);
			while(1);
		}
	}

	erase_start = addr & ~(block_size - 1);
	erase_end = (addr + len + block_size - 1) / block_size * block_size;

	/* 只测试 oob 区域 */
	if ((len == 0) && (oob_len > 0))
		erase_end = erase_start + block_size;

	gxflash_erasedata(erase_start, erase_end - erase_start);
	printf("sflash erase finish.\n");

	printf("read addr=0x%x, len=0x%x, oob_addr=0x%x, oob_len=0x%x, mode=%d\n", addr, len, oob_addr, oob_len, mode);
	gxflash_read_data_and_oob(addr, p, len, oob_addr, p1, oob_len, mode);
	printf("sflash_gx_readl finish.\n");
	for(i = 0, p_tmp = p; i < len; i++)
	{
		//printf("0x%x ", *p_tmp++);
		if(*p_tmp++ != 0xff)	/* 确认擦除后数据是否为全0xFF */
		{
			printf("erase error, i = %d, p[i]=0x%x \n", i, p[i]);
			//break;
			while(1);		//擦除错误则停止，便于打印观看
		}
	}
	for(i = 0, p_tmp = p1; i < oob_len; i++)
	{
		//printf("0x%x ", *p_tmp++);
		if(*p_tmp++ != 0xff)	/* 确认擦除后数据是否为全0xFF */
		{
			printf("erase oob error, i = %d, p1[i]=0x%x \n", i, p1[i]);
			//break;
			while(1);		//擦除错误则停止，便于打印观看
		}
	}

	if(len > 0) {
		p2 = malloc(len);
		if(p2 == NULL)
		{
			printf("LINE%d:malloc failed.malloc len=0x%x\n", __LINE__, len);
			while(1);
		}
		for (i = 0; i < len; i++) {
			p2[i] = rand_test();
		}
	}
	if(oob_len > 0) {
		p3 = malloc(oob_len);
		if(p3 == NULL)
		{
			printf("LINE%d:malloc failed.malloc ooblen=0x%x\n", __LINE__, oob_len);
			while(1);
		}
		for (i = 0; i < oob_len; i++) {
			p3[i] = rand_test();
		}
	}
	gxflash_write_data_and_oob(addr, p2, len, oob_addr, p3, oob_len, mode);
	printf("sflash page program finish.\n");
	memset(p, 0, len);
	memset(p1, 0, oob_len);
	gxflash_read_data_and_oob(addr, p, len, oob_addr, p1, oob_len, mode);
	printf("sflash_gx_readl finish.\n");
	/*for(i = 0, p_tmp = p; i < len; i++)
	{
		printf("0x%x ", *p_tmp++);
	}
	printf("\nprintf finish.\n");*/
	p_src = p;
	p_dst = p2;
	for(i = 0; i < len; i++)
	{
		if((*p_src++) != (*p_dst++))
		{
			printf("cmp error. i = %d. read=0x%x, orig=0x%x\n", i, *(p_src-1), *(p_dst-1));
			while(1);
			free(p);
			free(p2);
			return -1;
		}
	}
	printf("cmp finish. src equal dst.\n");
	free(p);
	free(p2);

	// mode = PLACE_OOB do not cmp
	if(mode != 0) {
		p_src = p1;
		p_dst = p3;
		for(i = 0; i < oob_len; i++)
		{
			if((*p_src++) != (*p_dst++))
			{
				printf("cmp oob error. i = %d. read=0x%x, orig=0x%x\n", i, *(p_src-1), *(p_dst-1));
				while(1);
				free(p1);
				free(p3);
				return -1;
			}
		}
	}
	printf("cmp oob finish. src equal dst.\n");
	free(p1);
	free(p3);

	return 0;
}

void flash_nand_complete_test(uint32_t count)
{
	int i, num, flash_size, erase_size, erase_num, page_size, oob_size;
	unsigned int data, mode;
	uint32_t t = 0;

	data = gxflash_get_info(GX_FLASH_CHIP_TYPE);
	flash_size = gxflash_get_info(GX_FLASH_CHIP_SIZE);
	erase_size = gxflash_get_info(GX_FLASH_ERASE_SIZE);
	page_size = gxflash_get_info(GX_FLASH_PAGE_SIZE);
	erase_num  = gxflash_get_info(GX_FLASH_ERASE_NUM);
	oob_size  = gxflash_get_info(GX_FLASH_OOB_SIZE);
	oob_size = oob_size / 2 - 6; // 防止测试的 oobsize 超过 oobfree.length

	switch(data){
		case GX_FLASH_CHIP_TYPE_NOR:		//spi nor
			printf("error: not support nor flash.\n");
			return;
		case GX_FLASH_CHIP_TYPE_SPINAND:		//spi nand
		case GX_FLASH_CHIP_TYPE_NAND:		//nand
			break;
		default:
			printf("error: cann't find flash type.\n");
			return;
	}
	gxflash_write_protect_unlock();

	while(t < count)	/*随机测试*/
	{
		uint32_t addr_tmp, len_tmp, oob_addr_tmp, oob_len_tmp;
		addr_tmp = flash_size * ((float)rand_test()/(0x7fff+1) );
		len_tmp = page_size * ((float)rand_test()/(0x7fff+1));
		oob_addr_tmp = oob_size * ((float)rand_test()/(0x7fff+1) );
		oob_len_tmp = oob_size * ((float)rand_test()/(0x7fff+1));
		//addr_tmp += 0x100000;

		uint32_t addr_offs = 0x1000 * ((float) rand_test()/(0x7fff+1));
		addr_tmp += addr_offs;
		if (addr_tmp + len_tmp > flash_size - erase_size * erase_num * 2 / 100)
			continue;
		if(oob_addr_tmp + oob_len_tmp > oob_size)
			continue;
		if(gxflash_block_isbad(addr_tmp))
			continue;

		mode = 2;  // OOB_MODE_RAW
		printf(">>>>>flash_nand_test:addr=0x%x, len = 0x%x, oob_addr=0x%x, oob_len=0x%x, mode=%u\n", addr_tmp, len_tmp, oob_addr_tmp, oob_len_tmp, mode);
		if(flash_nand_test_single(addr_tmp, len_tmp, oob_addr_tmp, oob_len_tmp, mode))
		{
			printf("*****flash_nand_test_failed:addr=0x%x, len = 0x%x, times=%d\n\n\n", \
				addr_tmp, len_tmp, t);
			//continue;
			while(1);
		}
		mode = 1;  // OOB_MODE_AUTO_OOB
		printf(">>>>>flash_nand_test:addr=0x%x, len = 0x%x, oob_addr=0x%x, oob_len=0x%x, mode=%u\n", addr_tmp, len_tmp, oob_addr_tmp, oob_len_tmp, mode);
		if(flash_nand_test_single(addr_tmp, len_tmp, oob_addr_tmp, oob_len_tmp, mode))
		{
			printf("*****flash_nand_test_failed:addr=0x%x, len = 0x%x, times=%d\n\n\n", \
				addr_tmp, len_tmp, t);
			//continue;
			while(1);
		}
		mode = 0;  // OOB_MODE_PLACE_OOB
		printf(">>>>>flash_nand_test:addr=0x%x, len = 0x%x, oob_addr=0x%x, oob_len=0x%x, mode=%u\n", addr_tmp, len_tmp, oob_addr_tmp, oob_len_tmp, mode);
		if(flash_nand_test_single(addr_tmp, len_tmp, oob_addr_tmp, oob_len_tmp, mode))
		{
			printf("*****flash_nand_test_failed:addr=0x%x, len = 0x%x, times=%d\n\n\n", \
				addr_tmp, len_tmp, t);
			//continue;
			while(1);
		}
		t++;
		printf(">>>>>flash_nand_test_success, %d.\n\n\n", t);
	}
	printf("flash nand comtest finished.\n");
}

typedef struct sflash_test_struct_s{
	uint32_t 	addr;
	uint32_t 	len;
}sflash_test_struct_t;
sflash_test_struct_t sflash_test_array[] = {
	{0x14000, 112350}, {0x15000, 112350}, {0x15000, 131072}, {0x1a0c0, 112350}, {0x1a000, 112352}, {0x1a0c0, 112352}, \
	{0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}, {0, 6}, {0, 7}, /*测试少数字节的读写*/	\
	{1, 1}, {1, 2}, {1, 3}, {1, 4}, {1, 5}, {1, 6}, {1, 7}, {1, 8}, {1, 9}, {1, 10}, /*测试未对齐的读写*/	\
	{0, 100}, {0, 256}, {0, 512}, /*测试稍多字节的读写*/	\
	{100, 256}, {100,100}, {200, 100}, {300, 300}, {100, 500}, /*测试跨PAGE读写*/	\
	{0, 0x1000}, {0, 0x1000+1}, {0, 0x1000+2}, {0, 0x1000+3}, {0, 0x1000+4}, /*测试多字节读写*/	\
	{0, 0x100000}, {0, 0x200000}, //{0, 0x400000} /*测试M级别字节读写*/		\
	/*此外还将产生随机地址和随机大小进行测试*/
};
sflash_test_struct_t nandflash_test_array[] = {
	{0x1000, 0x1000}, {0x0, 0x10000}, {0x10000, 0x10000}, {0x80000, 0x10000}, \
	{0x100000, 0x100000}, {0x60000, 0x800000}, {0x60000, 0x10000}, \
	{0x50000, 0x10000}, {0x30000, 0x10000}, {0x20000, 0x30000}
};

int flash_complete_test(unsigned int count)
{
	sflash_test_struct_t *p_array;
	int i, num, flash_size, erase_size, erase_num;
	unsigned int data;
	uint32_t t = 0;

	data = gxflash_get_info(GX_FLASH_CHIP_TYPE);
	flash_size = gxflash_get_info(GX_FLASH_CHIP_SIZE);
	erase_size = gxflash_get_info(GX_FLASH_ERASE_SIZE);
	erase_num  = gxflash_get_info(GX_FLASH_ERASE_NUM);

	switch(data){
		case GX_FLASH_CHIP_TYPE_NOR:		//spi nor
			num = sizeof(sflash_test_array) / sizeof(sflash_test_struct_t);
			p_array = sflash_test_array;
			break;
		case GX_FLASH_CHIP_TYPE_SPINAND:		//spi nand
		case GX_FLASH_CHIP_TYPE_NAND:		//nand
			num = sizeof(nandflash_test_array) / sizeof(sflash_test_struct_t);
			p_array = nandflash_test_array;
			break;
		default:
			printf("error: cann't find flash type.\n");
			return;
	}

	if (sample_delay_test == 0)
		gxflash_write_protect_unlock();

	for(i = 0; i < num; i++)
	{
		printf(">>>>>sflash_test:addr=0x%x, len = 0x%x\n", \
			p_array[i].addr, p_array[i].len);
		if(sflash_test_single(p_array[i].addr, p_array[i].len))
		{
			printf("*****sflash_test_failed:addr=0x%x, len = 0x%x\n\n\n", \
				p_array[i].addr, p_array[i].len);
			if (sample_delay_test == 0) {
				//continue;
				while(1);
			} else {
				return -1;
			}
		}
		printf(">>>>>sflash_test_success.\n\n\n");
	}

	//gx_gpio_setlevel(20, 0);

	while(t < count)	/*随机测试*/
	{
		uint32_t addr_tmp, len_tmp;
		addr_tmp = flash_size * ((float)rand_test()/(0x7fff+1) );
		len_tmp = RAND_TEST_LEN_MAX * ((float)rand_test()/(0x7fff+1));

		if (data == GX_FLASH_CHIP_TYPE_NOR) {
			if (addr_tmp + len_tmp > flash_size)
				continue;
		} else {
			uint32_t addr_offs = 0x1000 * ((float) rand_test()/(0x7fff+1));
			addr_tmp += addr_offs;
			if (addr_tmp + len_tmp > flash_size - erase_size * erase_num * 2 / 100)
				continue;
		}

		printf(">>>>>sflash_test:addr=0x%x, len = 0x%x\n", addr_tmp, len_tmp);
		if(sflash_test_single(addr_tmp, len_tmp))
		{
			printf("*****sflash_test_failed:addr=0x%x, len = 0x%x, times=%d\n\n\n", \
				addr_tmp, len_tmp, t);
			if (sample_delay_test == 0) {
				//continue;
				while(1);
			} else {
				return -1;
			}
		}
		t++;
		printf(">>>>>sflash_test_success, %d.\n\n\n", t);
	}
	printf("flash comtest finished.\n");

	return 0;
}

//#define BLOCK_SIZE 0x20000
static void imitate_block(int bad_block)
{
	unsigned int block_size = 0;

	block_size = gxflash_get_info(GX_FLASH_BLOCK_SIZE);
	int bad_addr = bad_block * block_size;

	gxflash_block_markbad(bad_addr);

	if(gxflash_block_bbt_isbad(bad_addr)) {
		printf("mark bad block succeed, addr is 0x%x\n", bad_addr);
	} else {
		printf("mark bad block failed, addr is 0x%x\n", bad_addr);
	}
	//update bbt
	gxflash_badinfo();
}

void mark_bad_block_test(unsigned int addr)
{
	unsigned int block_size = 0;

	unsigned int data;

	data = gxflash_get_info(GX_FLASH_CHIP_TYPE);

	if (data == GX_FLASH_CHIP_TYPE_NOR) {
		printf("only support nand, exit\n");
		return;
	}
	block_size = gxflash_get_info(GX_FLASH_BLOCK_SIZE);
	unsigned int bad_block_id = addr / block_size;

	gxflash_write_protect_unlock();
	imitate_block(bad_block_id);
}

void recover_bad_block_test(unsigned int addr)
{
	unsigned int block_size = 0, chip_size = 0;

	unsigned int data;

	data = gxflash_get_info(GX_FLASH_CHIP_TYPE);

	if (data == GX_FLASH_CHIP_TYPE_NOR) {
		printf("only support nand, exit\n");
		return;
	}
	block_size = gxflash_get_info(GX_FLASH_BLOCK_SIZE);
	chip_size = gxflash_get_info(GX_FLASH_CHIP_SIZE);
	unsigned int bad_addr = addr & ~(block_size-1);

	gxflash_write_protect_unlock();
	gxflash_scruberase(bad_addr, block_size);
	gxflash_scruberase(chip_size-4*block_size, 4*block_size);
	gxflash_scruberase(chip_size-4*block_size, 4*block_size);

	//update bbt
	gxflash_badinfo();

	if(gxflash_block_bbt_isbad(bad_addr) == 0) {
		printf("recover bad block succeed, addr is 0x%x\n", bad_addr);
	} else {
		printf("recover bad block failed, addr is 0x%x\n", bad_addr);
	}
}

static int flash_bbt_block_test(void)
{
#define TEST_BLOCK_NUM 20
#define TEST_BYTES_PER_BLOCK 0x2000
	uint8_t *src[TEST_BLOCK_NUM];
	uint8_t *dst[TEST_BLOCK_NUM];
	int i;
	int test_block_num = TEST_BLOCK_NUM;
	int test_len = TEST_BYTES_PER_BLOCK;
	unsigned int bad_block, bad_addr;

	if (gxflash_badinfo() != 0) {
		printf("this type of flash doesn't support oob!\n");
		return -1;
	}

	unsigned int block_size = 0;
	block_size = gxflash_get_info(GX_FLASH_BLOCK_SIZE);

	//imitate block 5 with bad one
	bad_block = 5;
	bad_addr = bad_block * block_size;

	for (i=0; i<test_block_num; i++) {
		src[i] = malloc(test_len);
		dst[i] = malloc(test_len);
		if(src[i] == NULL || dst[i] == NULL) {
			printf("LINE%d:malloc failed.\n", __LINE__);
			while(1);
		}
	}

	for (i=0; i<test_block_num; i++) {
		memset(src[i], i, test_len);
	}

	/* write & read with skip */
	for (i=0; i<test_block_num; i++) {
		memset(dst[i], 0x5a, test_len);
	}
	gxflash_erasedata_noskip(0, (test_block_num+1)*block_size);
	imitate_block(bad_block);

	for (i=0; i<bad_block+1; i++) {
		gxflash_pageprogram(i*block_size, src[i], test_len);
		gxflash_readdata(i*block_size, dst[i], test_len);
	}
	gxflash_erasedata_noskip((bad_block+1)*block_size, block_size);//must be erased first
	for (i=bad_block+1; i<test_block_num; i++) {
		gxflash_pageprogram(i*block_size, src[i], test_len);
		gxflash_readdata(i*block_size, dst[i], test_len);
	}
	for (i=0; i<test_block_num; i++) {
		if (memcmp(src[i], dst[i], test_len) != 0) {
			printf("LINE%d:write failed. Block %d\n", __LINE__, i);
			while(1);
		}
	}
	printf(">>>>>sflash write read with skip ok.\n\n\n");

	/* write & read with no skip */
	for (i=0; i<test_block_num; i++) {
		memset(dst[i], 0x5a, test_len);
	}
	gxflash_erasedata_noskip(0, (test_block_num+1)*block_size);
	imitate_block(bad_block);

	for (i=0; i<test_block_num; i++) {
		gxflash_pageprogram_noskip(i*block_size, src[i], test_len);
		gxflash_readdata_noskip(i*block_size, dst[i], test_len);
	}
	for (i=0; i<test_block_num; i++) {
		if (memcmp(src[i], dst[i], test_len) != 0) {
			printf("LINE%d:write failed. Block %d\n", __LINE__, i);
			while(1);
		}
	}
	printf(">>>>>sflash write read with no skip ok.\n\n\n");

	/* write with skip, read with no skip */
	for (i=0; i<test_block_num; i++) {
		memset(dst[i], 0x5a, test_len);
	}
	gxflash_erasedata_noskip(0, (test_block_num+1)*block_size);
	imitate_block(bad_block);

	for (i=0; i<bad_block+1; i++) {
		gxflash_pageprogram(i*block_size, src[i], test_len);
		gxflash_readdata_noskip(i*block_size, dst[i], test_len);
	}
	gxflash_erasedata_noskip((bad_block+1)*block_size, block_size);//must be erased first
	for (i=bad_block+1; i<test_block_num; i++) {
		gxflash_pageprogram(i*block_size, src[i], test_len);
		gxflash_readdata_noskip(i*block_size, dst[i], test_len);
	}

	for (i=0; i<bad_block; i++) {
		if (memcmp(src[i], dst[i], test_len) != 0) {
			printf("LINE%d:write failed. Block %d\n", __LINE__, i);
			while(1);
		}
	}
	for (i=0; i<test_len; i++) {
		if (dst[bad_block][i] != 0xff) {
			printf("LINE%d:write failed. Block %d\n", __LINE__, bad_block);
			while(1);
		}
	}
	for (i=bad_block+1; i<test_block_num; i++) {
		if (memcmp(src[i], dst[i], test_len) != 0) {
			printf("LINE%d:write failed. Block %d\n", __LINE__, i);
			while(1);
		}
	}
	printf(">>>>>sflash write with skip, read with no skip ok.\n\n\n");

	/* write with no skip, read with skip */
	for (i=0; i<test_block_num; i++) {
		memset(dst[i], 0x5a, test_len);
	}
	gxflash_erasedata_noskip(0, (test_block_num+1)*block_size);
	imitate_block(bad_block);

	for (i=0; i<bad_block+1; i++) {
		gxflash_pageprogram_noskip(i*block_size, src[i], test_len);
		gxflash_readdata(i*block_size, dst[i], test_len);
	}
	gxflash_erasedata_noskip((bad_block+1)*block_size, block_size);//must be erased first
	for (i=bad_block+1; i<test_block_num; i++) {
		gxflash_pageprogram_noskip(i*block_size, src[i], test_len);
		gxflash_readdata(i*block_size, dst[i], test_len);
	}
	for (i=0; i<bad_block; i++) {
		if (memcmp(src[i], dst[i], test_len) != 0) {
			printf("LINE%d:write failed. Block %d\n", __LINE__, i);
			while(1);
		}
	}
	for (i=0; i<test_len; i++) {
		if (dst[bad_block][i] != 0xff) {
			printf("LINE%d:write failed. Block %d\n", __LINE__, bad_block);
			while(1);
		}
	}
	for (i=bad_block+1; i<test_block_num; i++) {
		if (memcmp(src[i], dst[i], test_len) != 0) {
			printf("LINE%d:write failed. Block %d\n", __LINE__, i);
			while(1);
		}
	}
	printf(">>>>>sflash write with no skip, read with skip ok.\n\n\n");

	//recover bad block
	gxflash_erasedata_noskip(bad_addr, block_size);

	for (i=0; i<test_block_num; i++) {
		free(src[i]);
		free(dst[i]);
	}

	return 0;
}

static int flash_bbt_partition_test(void)
{
	uint8_t *src, *dst;
	unsigned int addr, len;
	unsigned int bad_block, bad_addr;

	if (gxflash_badinfo() != 0) {
		printf("this type of flash doesn't support oob!\n");
		return -1;
	}

	unsigned int block_size = 0;
	block_size = gxflash_get_info(GX_FLASH_BLOCK_SIZE);

	//imitate block 5 with bad one
	bad_block = 5;
	bad_addr = bad_block * block_size;

	//block 4, 5, 6
	addr = 4*block_size;
	len = 3*block_size;

	src = malloc(len);
	dst = malloc(len);
	if(src == NULL || dst == NULL)
	{
		printf("LINE%d:malloc failed.malloc len=0x%x\n", __LINE__, len);
		while(1);
	}

	memset(src, 0, block_size);
	memset(src+block_size, 1, block_size);
	memset(src+2*block_size, 2, block_size);

	gxflash_erasedata_noskip(addr, len+block_size);
	imitate_block(bad_block);
	gxflash_pageprogram_noskip(addr, src, len);
	gxflash_readdata(addr, dst, len);
	if (memcmp(src, dst, block_size) != 0 || memcmp(src+2*block_size, dst+block_size, block_size) != 0) {
		printf("LINE%d:write failed.\n", __LINE__);
		while(1);
	}

	gxflash_erasedata_noskip(addr, len+block_size);
	imitate_block(bad_block);
	gxflash_pageprogram(addr, src, len);
	gxflash_readdata_noskip(addr, dst, len);
	if (memcmp(src, dst, block_size) != 0 || memcmp(src+block_size, dst+2*block_size, block_size) != 0) {
		printf("LINE%d:write failed.\n", __LINE__);
		while(1);
	}

	gxflash_erasedata_noskip(addr, len+block_size);
	imitate_block(bad_block);
	gxflash_pageprogram_noskip(addr, src, len);
	gxflash_readdata_noskip(addr, dst, len);
	if (memcmp(src, dst, 0x60000) != 0) {
		printf("LINE%d:write failed.\n", __LINE__);
		while(1);
	}

	gxflash_erasedata_noskip(addr, len+block_size);
	imitate_block(bad_block);
	gxflash_pageprogram(addr, src, len);
	gxflash_readdata(addr, dst, len);
	if (memcmp(src, dst, 0x60000) != 0) {
		printf("LINE%d:write failed.\n", __LINE__);
		while(1);
	}

	//recover bad block
	gxflash_erasedata_noskip(bad_addr, block_size);

	free(src);
	free(dst);

	return 0;
}

void flash_oob_test(void)
{
	gxflash_write_protect_unlock();

	printf(">>>>>nand flash bbt block test start.\n");
	if (flash_bbt_block_test() != 0)
		return;
	printf(">>>>>nand flash bbt block test successfully.\n\n\n");

	printf(">>>>>nand flash bbt partition test start.\n");
	if (flash_bbt_partition_test() != 0)
		return;
	printf(">>>>>nand flash bbt partition test successfully.\n\n\n");
}

static int flash_wp_program_single_test(unsigned int length, unsigned int offs)
{
	unsigned int block_size;
	unsigned int flash_size, i, actual_protect;
	unsigned char mark = 0xab, tmp;

	actual_protect = 0;
	block_size = gxflash_get_info(GX_FLASH_BLOCK_SIZE);
	flash_size= gxflash_get_info(GX_FLASH_CHIP_SIZE);

	printf("test protect size 0x%x, driver: ", length);
	gxflash_write_protect_lock(length);

	for (i = 0; i < flash_size; i += block_size) {
		gxflash_pageprogram(i+offs, &mark, sizeof(mark));
	}

	for (i = 0; i < flash_size; i += block_size) {
		gxflash_readdata(i+offs, &tmp, sizeof(tmp));
		if (tmp == 0xff) {
			actual_protect += block_size;
		} else {
			break;
		}
	}

	for (; i < flash_size; i += block_size) {
		gxflash_readdata(i+offs, &tmp, sizeof(tmp));
		if (tmp != mark) {
			printf("0x%07x write data compare fail\n", i);
		}
	}

	if (actual_protect == length)
		printf("protect size 0x%x, OK\n", length);
	else
		printf("protect size 0x%x, actual protect 0x%x, NG\n", length, actual_protect);
}

static int flash_wp_erase_single_test(unsigned int length)
{
	unsigned int block_size;
	unsigned int flash_size, i, actual_protect;
	unsigned char mark = 0xcd, tmp;

	actual_protect = 0;
	block_size = gxflash_get_info(GX_FLASH_BLOCK_SIZE);
	flash_size= gxflash_get_info(GX_FLASH_CHIP_SIZE);

	printf("erase all flash, wait..., ");
	gxflash_write_protect_lock(0);
	gxflash_erasedata(0, flash_size);

	for (i = 0; i < flash_size; i += block_size) {
		gxflash_pageprogram(i, &mark, sizeof(mark));
	}

	printf("test protect size 0x%x, driver: ", length);
	gxflash_write_protect_lock(length);
	gxflash_erasedata(0, flash_size);

	for (i = 0; i < flash_size; i += block_size) {
		gxflash_readdata(i, &tmp, sizeof(tmp));
		if (tmp == mark) {
			actual_protect += block_size;
		} else {
			break;
		}
	}

	for (; i < flash_size; i += block_size) {
		gxflash_readdata(i, &tmp, sizeof(tmp));
		if (tmp != 0xff) {
			printf("***0x%07x erase data compare fail***\n", i);
		}
	}

	if (actual_protect == length)
		printf("protect size 0x%x, OK\n", length);
	else
		printf("protect size 0x%x, actual protect 0x%x, NG\n", length, actual_protect);
}

void flash_wplock(int len)
{
	gxflash_write_protect_lock(len);
}

void flash_wp_status(void)
{
	unsigned int len;
	gxflash_write_protect_status(&len);

	printf("protect length: 0x%x\n", len);
}

void flash_wp_test(void)
{
	unsigned int flash_size, protect_size, i, mark_offs;

	flash_size= gxflash_get_info(GX_FLASH_CHIP_SIZE);
	mark_offs = 0;

	printf("erase all flash, wait...\n");
	gxflash_erasedata(0, flash_size);
	printf("erase finish\n");
	printf("===start program protect test===\n");

	flash_wp_program_single_test(0, mark_offs++);

	for (i = 6; i > 0; i--) {
		protect_size = flash_size >> i;
		flash_wp_program_single_test(protect_size, mark_offs++);
	}

	for (i = 2; i <= 6; i++) {
		protect_size =  flash_size - (flash_size >> i);
		flash_wp_program_single_test(protect_size, mark_offs++);
	}

	flash_wp_program_single_test(flash_size, mark_offs++);

	printf("===start erase protect test===\n");
	for (i = 6; i > 0; i--) {
		protect_size = flash_size >> i;
		flash_wp_erase_single_test(protect_size);
	}

	for (i = 2; i <= 6; i++) {
		protect_size =  flash_size - (flash_size >> i);
		flash_wp_erase_single_test(protect_size);
	}

	flash_wp_erase_single_test(flash_size);

	printf("unlock protect\n");
	gxflash_write_protect_lock(0);
}

int flash_nor_sector_erase_test(void)
{
	unsigned int flash_size, erase_size, erase_num, block_size, block_num;
	unsigned int sector_size, sector_num_per_block;
	int ret = 0, i, j;
	uint8_t *mem, *mirror_mem;
	unsigned int block, sector;
	unsigned int erase_addr;
	uint8_t random_data;

	flash_size  = gxflash_get_info(GX_FLASH_CHIP_SIZE);
	erase_size  = gxflash_get_info(GX_FLASH_ERASE_SIZE);
	erase_num   = gxflash_get_info(GX_FLASH_ERASE_NUM);
	block_size  = 0x10000;
	block_num   = flash_size / block_size;
	sector_size = gxflash_get_info(GX_FLASH_SECTOR_SIZE);
	sector_num_per_block = block_size / sector_size;

	printf("erase size: 0x%x, block size: 0x%x, secotr size: 0x%x\n", \
			erase_size, block_size, sector_size);

	/* 4K sector擦除大小测试 */
	if (erase_size != 0x1000) {
		printf("erase size is not 4KB, exit\n");
		return -1;
	}

	mem = malloc(block_size);
	if (mem == NULL) {
		printf("%s: %d, malloc memory fail\n", __func__, __LINE__);
		return -1;
	}

	mirror_mem = malloc(block_size);
	if (mirror_mem == NULL) {
		printf("%s: %d, malloc memory fail\n", __func__, __LINE__);
		free(mem);
		return -1;
	}

	for (i = 0; i < 16; i++) {
		block  = rand_test() % block_num;
		sector = rand_test() % sector_num_per_block;

		printf("%d, test block at 0x%x, sector offs 0x%x\n", \
				i, block_size * block, sector_size * sector);
		printf("erase one random block, wait...\n");
		gxflash_erasedata(block_size * block, block_size);
		memset(mirror_mem, 0xFF, block_size);

		for (j = 0; j < block_size; j++) {
			random_data    = rand_test();
			mem[j]         = random_data;
			mirror_mem[j] &= random_data;
		}

		gxflash_pageprogram(block_size * block, mem, block_size);

		memset(mem, 0x00, block_size);
		gxflash_readdata(block_size * block, mem, block_size);

		for (j = 0; j < block_size; j++) {
			if (mem[j] != mirror_mem[j]) {
				printf("compare random data fail\n");
				ret = -1;
				goto EXIT;
			}
		}

		erase_addr = block_size * block +  sector_size * sector;
		gxflash_erasedata(erase_addr, sector_size);
		memset(mirror_mem + sector_size * sector, 0xFF, sector_size);

		memset(mem, 0x00, block_size);
		gxflash_readdata(block_size * block, mem, block_size);

		for (j = 0; j < block_size; j++) {
			if (mem[j] != mirror_mem[j]) {
				printf("compare erase sector test data fail\n");
				ret = -1;
				goto EXIT;
			}
		}


	}

	printf("test 4KB erase OK!\n");
EXIT:
	free(mem);
	free(mirror_mem);

	return ret;
}

int bit_diff(const uint8_t* src, const uint8_t* dst, int len)
{
	int i, j;
	int diff = 0;

	for (i = 0; i < len; i++) {
		if (src[i] != dst[i]) {
			for (j = 0; j < 8; j++) {
				if (((src[i] >> j) & 0x01) != ((dst[i] >> j) & 0x01))
					diff++;
			}
		}
	}

	return diff;
}

int bit_low_count(uint8_t* src, int len)
{
	int i, j;
	int diff = 0;

	for (i = 0; i < len; i++) {
		if (src[i] != 0xFF) {
			for (j = 0; j < 8; j++) {
				if (((src[i] >> j) & 0x01) == 0x00)
					diff++;
			}
		}
	}

	return diff;
}
/*
 * 用途：测试SPI NAND和并行NAND的ECC纠错能力，同时显示与原始数据不同的bit数
 * 原理：block的擦写次数是有限的，当超过擦写次数时，block中的一些bit会出现无法变化的现象，
 *       据此原理，可以将block进行大量次数的擦写，致某些bit坏掉，然后观察ECC是否能将这些坏掉的bit纠正回来。
 * 总结：此命令可以测试擦除页和随机数据页ECC的纠错能力。此测试方法特别适用于ECC不能关闭的Flash，
 *       缺点是测试时间很长。使用此命令时，需要将运行log记录下来，然后查找最小的不同bit数，
 *       如果此值比flash声称的小，则说明ECC有问题。
 */
int flash_nand_ecc_buildin_worn_test( uint32_t addr)
{
	int i, j, bitflips, block, round = 0, update = 0;
	unsigned int flash_size, erase_size, page_size, block_size, erase_num, block_addr;
	uint8_t *w_test_buffer, *r_test_buffer;
	int erase_corrected_min = -1, erase_corrected_most = 0;
	int random_corrected_min = -1, random_corrected_most = 0;

	flash_size = gxflash_get_info(GX_FLASH_CHIP_SIZE);
	erase_size = gxflash_get_info(GX_FLASH_ERASE_SIZE);
	page_size  = gxflash_get_info(GX_FLASH_PAGE_SIZE);
	erase_num  = gxflash_get_info(GX_FLASH_ERASE_NUM);
	block_size = gxflash_get_info(GX_FLASH_BLOCK_SIZE);

	w_test_buffer = malloc(page_size);
	if (w_test_buffer == NULL) {
		printf("%s: %d, malloc memory fail\n", __func__, __LINE__);
		return -1;
	}

	r_test_buffer = malloc(page_size);
	if (r_test_buffer == NULL) {
		printf("%s: %d, malloc memory fail\n", __func__, __LINE__);
		free(w_test_buffer);
		return -1;
	}

	memset(w_test_buffer, 0xFF, page_size);
	for (i = 0; i < 64; i++) {
		w_test_buffer[i] = rand_test();
		//printf("%02x ", w_test_buffer[i]);
	}

	block = rand_test() % (erase_num / 2) + 8;

	while (1) {
		if (gxflash_block_isbad(block * block_size))
			block++;
		else
			break;
	}

	if(addr > 0)
		block_addr = addr;
	else
		block_addr = block * block_size;

	printf("start test block_addr: 0x%x\n", block_addr);
	while(1) {
		round++;

		for (i = 0; i < page_size; i++) {
			w_test_buffer[i] = rand_test();
		}

		gxflash_erasedata(block_addr, block_size);

		for (j = 0; j < 64; j++) {
			gxflash_readdata(block_addr + page_size * j, r_test_buffer, page_size);
			bitflips = bit_low_count(r_test_buffer, page_size);

			if (bitflips != 0) {
				printf("erase page bitflips %d\n", bitflips);
				if (bitflips > erase_corrected_most) {
					erase_corrected_most = bitflips;
					update = 1;
				}

				if (erase_corrected_min == -1) {
					erase_corrected_min = bitflips;
					update = 1;
				} else {
					if (bitflips < erase_corrected_min) {
						erase_corrected_min = bitflips;
						update = 1;
					}
				}
			}
		}

		for (j = 0; j < 64; j++) {
			gxflash_pageprogram(block_addr + page_size * j, w_test_buffer, page_size);
		}

		for (j = 0; j < 64; j++) {
			gxflash_readdata(block_addr + page_size * j, r_test_buffer, page_size);
			bitflips = bit_diff(r_test_buffer, w_test_buffer, page_size);

			if (bitflips != 0) {
				printf("random data bitflips %d\n", bitflips);
				if (bitflips > random_corrected_most) {
					random_corrected_most = bitflips;
					update = 1;
				}
				if (random_corrected_min == -1) {
					random_corrected_min = bitflips;
					update = 1;
				} else {
					if (bitflips < random_corrected_min) {
						random_corrected_min = bitflips;
						update = 1;
					}
				}
			}
		}

		printf("round: %d\n", round);
		if (update) {
			update = 0;
			printf("round: %d, erase page ecc test [%d: %d], random data ecc test [%d: %d]\n", \
					round, erase_corrected_min, erase_corrected_most, \
					random_corrected_min, random_corrected_most);
		}
	}

	free(r_test_buffer);
	free(w_test_buffer);

	return 0;
}

static int reverse_bits(void *buf, int start, int bits_num)
{
	unsigned char *writebuf = buf;
	int counter = 0, i, bit = 0;

	i = start;

	while(counter  < bits_num) {
		for (bit = 0; bit < 8; bit++) {
			if (writebuf[i] & (1<<bit)) {
				writebuf[i] &= ~(1<<bit);
				counter++;
				if (counter == bits_num)
					break;
			}
		}
		i++;
	}

	return 0;
}

/*
 * 用途：伪造SPI NAND和并行NAND中数据bit翻转，测试驱动ECC逻辑是否正常。
 * 原理：关闭ECC情况下， 将指定空间的数据页起始位置一些bit位写0，以伪造数据bit翻转的现象
 * 总结：此测试方法仅适用于ECC能关闭的SPI NAND和NAND Flash。使用此命令时，可以指定位置、长度和bit翻转个数。
 */
int flash_nand_bitflip_test(unsigned int start, unsigned int len, int bitflips_num)
{
	unsigned int flash_size, erase_size, page_size, block_size, erase_num;
	uint8_t *w_test_buffer, *r_test_buffer;
	unsigned int pos, block_addr, test_end;

	flash_size = gxflash_get_info(GX_FLASH_CHIP_SIZE);
	erase_size = gxflash_get_info(GX_FLASH_ERASE_SIZE);
	page_size  = gxflash_get_info(GX_FLASH_PAGE_SIZE);
	erase_num  = gxflash_get_info(GX_FLASH_ERASE_NUM);
	block_size = gxflash_get_info(GX_FLASH_BLOCK_SIZE);

	w_test_buffer = malloc(page_size);
	if (w_test_buffer == NULL) {
		printf("%s: %d, malloc memory fail\n", __func__, __LINE__);
		return -1;
	}

	r_test_buffer = malloc(page_size);
	if (r_test_buffer == NULL) {
		printf("%s: %d, malloc memory fail\n", __func__, __LINE__);
		free(w_test_buffer);
		return -1;
	}

	gxflash_write_protect_unlock();

	pos = start & ~(page_size - 1);
	test_end = min(start + len, flash_size);

	while (pos < test_end) {
		block_addr = pos & ~(block_size - 1);

		if (gxflash_block_bbt_isbad(block_addr)) {
			block_addr += block_size;
			pos = (pos + block_size) & ~(block_size - 1);
			continue;
		}

		for (; pos < block_addr + block_size; pos += page_size) {
			if (pos >= test_end)
				break;
			gxflash_readdata_direct(pos, r_test_buffer, page_size);
			reverse_bits(r_test_buffer, 0, bitflips_num);
			gxflash_pageprogram_noecc(pos, r_test_buffer, page_size);
		}
	}

	free(r_test_buffer);
	free(w_test_buffer);

	return 0;
}

/*
 * 用途：测试SPI NAND和并行NAND的ECC纠错能力，最终显示ECC最大能纠错的bit数
 * 原理：在flash上随机选择一个block，擦除block，此时数据为全0xFF, 关闭ECC情况下，
 *       将起始地址的一些bit位写0，以伪造擦除后不全为1的情况，然后开启ECC，读取数据，观察数据是否全为0xFF。
 *       同理，在擦除的block上，写入带有ecc的随机数据，使用同样方法制造数据bit翻转的现象。
 *       逐步增加bit跳变个数，直到ECC不能纠正为止，即可得到ECC的纠错能力。
 * 总结：此命令可以测试擦除页和随机数据页ECC的纠错能力。此方法不适用于ECC不能关闭的NAND Flash
 */
int flash_nand_ecc_test(void)
{
	int i, j, bitflips, block, test_bits = 0, update = 0;
	unsigned int flash_size, erase_size, page_size, block_size, erase_num, block_addr;
	uint8_t *w_test_buffer, *r_test_buffer;
	int ecc_corrected;

	flash_size = gxflash_get_info(GX_FLASH_CHIP_SIZE);
	erase_size = gxflash_get_info(GX_FLASH_ERASE_SIZE);
	page_size  = gxflash_get_info(GX_FLASH_PAGE_SIZE);
	erase_num  = gxflash_get_info(GX_FLASH_ERASE_NUM);
	block_size = gxflash_get_info(GX_FLASH_BLOCK_SIZE);

	w_test_buffer = malloc(page_size);
	if (w_test_buffer == NULL) {
		printf("%s: %d, malloc memory fail\n", __func__, __LINE__);
		return -1;
	}

	r_test_buffer = malloc(page_size);
	if (r_test_buffer == NULL) {
		printf("%s: %d, malloc memory fail\n", __func__, __LINE__);
		free(w_test_buffer);
		return -1;
	}

	memset(w_test_buffer, 0xFF, page_size);
	for (i = 0; i < 64; i++) {
		w_test_buffer[i] = rand_test();
		//printf("%02x ", w_test_buffer[i]);
	}

	block = rand_test() % (erase_num / 2) + 8;

	while (1) {
		if (gxflash_block_isbad(block * block_size))
			block++;
		else
			break;
	}

	block_addr = block * block_size;

	while(1) {
		test_bits++;

		printf("test erase fuction, test_bits: %d\n", test_bits);
		gxflash_erasedata(block_addr, block_size);

		ecc_corrected = 0;
		for (j = 0; j < 64; j++) {
			gxflash_readdata(block_addr + page_size * j, r_test_buffer, page_size);
			bitflips = bit_low_count(r_test_buffer, page_size);

			if (bitflips == 0)
				ecc_corrected = 1;
		}

		if (ecc_corrected == 0) {
			printf("1, erase page bitflips %d\n", bitflips);
			goto EXIT;
		}

		for (i = 0; i < page_size; i++) {
			w_test_buffer[i] = 0xFF;
		}

		reverse_bits(w_test_buffer, 0, test_bits);

		for (j = 0; j < 64; j++) {
			gxflash_pageprogram_noecc(block_addr + page_size * j, w_test_buffer, page_size);
		}

		ecc_corrected = 0;
		for (j = 0; j < 64; j++) {
			gxflash_readdata(block_addr + page_size * j, r_test_buffer, page_size);
			bitflips = bit_low_count(r_test_buffer, page_size);

			if (bitflips == 0)
				ecc_corrected = 1;
		}

		if (ecc_corrected == 0) {
			printf("%d, erase page bitflips %d\n", __LINE__, bitflips);
			goto EXIT;
		}

		printf("test program fuction, test_bits: %d\n", test_bits);
		gxflash_erasedata(block_addr, block_size);

		ecc_corrected = 0;
		for (j = 0; j < 64; j++) {
			gxflash_readdata(block_addr + page_size * j, r_test_buffer, page_size);
			bitflips = bit_low_count(r_test_buffer, page_size);

			if (bitflips == 0)
				ecc_corrected = 1;
		}

		if (ecc_corrected == 0) {
			printf("%d, erase page bitflips %d\n", __LINE__, bitflips);
			goto EXIT;
		}

		for (i = 0; i < page_size; i++) {
			w_test_buffer[i] = rand_test();
			r_test_buffer[i] = w_test_buffer[i];
		}

		for (j = 0; j < 64; j++) {
			gxflash_pageprogram(block_addr + page_size * j, w_test_buffer, page_size);
		}

		reverse_bits(w_test_buffer, 0, test_bits);

		for (j = 0; j < 64; j++) {
			gxflash_pageprogram_noecc(block_addr + page_size * j, w_test_buffer, page_size);
		}

		ecc_corrected = 0;
		for (j = 0; j < 64; j++) {
			memset(w_test_buffer, 0xFF, page_size);
			gxflash_readdata(block_addr + page_size * j, w_test_buffer, page_size);
			bitflips = bit_diff(r_test_buffer, w_test_buffer, page_size);

			if (bitflips == 0)
				ecc_corrected = 1;
		}

		if (ecc_corrected == 0) {
			printf("%d, test bitflips %d, random data page bitflips %d\n", __LINE__, test_bits, bitflips);
			goto EXIT;
		}
	}

EXIT:
	if (test_bits == 0)
		printf("bitflips test failed\n");
	else if (test_bits == 1)
		printf("ECC test fail, the ECC may not be turn off\n", test_bits - 1);
	else
		printf("bitflips correct strength is %d\n", test_bits - 1);


	free(r_test_buffer);
	free(w_test_buffer);

	return 0;
}

/* 联合存储SPINAND，ecc纠错能力测试case
 * 其SPINAND不能够对flash同一个page页在不erase的情况下进行二次program，如果二次program，会不起作用，并影响page页内数据跳变
 * 根据其提供测试ecc的方法，实现如下测试case，功能为：
 * 选取一个block，擦除block， 生成一页大小随机数据；
 * 向block中第一个page写入随机数据，读取第一个page的oob区域数据(ecc code)保持起来；
 * 关闭ecc功能，对随机数据进行位翻转，bit1 > bit0，将翻转后的随机数据写入到第二页到最后一页内，并同时写入保存的oob数据；
 * 开启ecc功能，读取第二页到最后一页数据与原始随机数据进行比较，判断位翻是否纠错成功，得出纠错能力；
 *
 * */
int flash_nand_ecc2_test(void)
{
	int i, j, bitflips, block, test_bits = 0, update = 0;
	unsigned int flash_size, erase_size, page_size, block_size, erase_num, block_addr, oob_size;
	uint8_t *w_test_buffer, *r_test_buffer, *oob_buffer, *page_buffer;
	int ecc_corrected;

	flash_size = gxflash_get_info(GX_FLASH_CHIP_SIZE);
	erase_size = gxflash_get_info(GX_FLASH_ERASE_SIZE);
	page_size  = gxflash_get_info(GX_FLASH_PAGE_SIZE);
	erase_num  = gxflash_get_info(GX_FLASH_ERASE_NUM);
	block_size = gxflash_get_info(GX_FLASH_BLOCK_SIZE);
	oob_size    = gxflash_get_info(GX_FLASH_OOB_SIZE);

	if(oob_size != 64 && oob_size != 128 && oob_size != 112 && oob_size != 256) {
		printf("oob size: %u error!\n", oob_size);
		return -1;
	}

	w_test_buffer = malloc(page_size);
	if (w_test_buffer == NULL) {
		printf("%s: %d, malloc memory fail\n", __func__, __LINE__);
		return -1;
	}

	r_test_buffer = malloc(page_size);
	if (r_test_buffer == NULL) {
		printf("%s: %d, malloc memory fail\n", __func__, __LINE__);
		free(w_test_buffer);
		return -1;
	}

	oob_buffer = malloc(oob_size);
	if (oob_buffer == NULL) {
		printf("%s: %d, malloc memory fail\n", __func__, __LINE__);
		free(w_test_buffer);
		free(r_test_buffer);
		return -1;
	}

	page_buffer = malloc(page_size+oob_size);
	if (page_buffer == NULL) {
		printf("%s: %d, malloc memory fail\n", __func__, __LINE__);
		free(w_test_buffer);
		free(r_test_buffer);
		free(oob_buffer);
		return -1;
	}
	memset(w_test_buffer, 0xFF, page_size);
	for (i = 0; i < 64; i++) {
		w_test_buffer[i] = rand_test();
		//printf("%02x ", w_test_buffer[i]);
	}

	block = rand_test() % (erase_num / 2) + 8;

	while (1) {
		if (gxflash_block_isbad(block * block_size))
			block++;
		else
			break;
	}

	block_addr = block * block_size;

	while(1) {
		test_bits++;

		printf("test program fuction, test_bits: %d\n", test_bits);
		gxflash_erasedata(block_addr, block_size);

		ecc_corrected = 0;
		for (j = 0; j < 64; j++) {
			gxflash_readdata(block_addr + page_size * j, r_test_buffer, page_size);
			bitflips = bit_low_count(r_test_buffer, page_size);

			if (bitflips == 0)
				ecc_corrected = 1;
		}

		if (ecc_corrected == 0) {
			printf("1, erase page bitflips %d\n", bitflips);
			goto EXIT;
		}

		for (i = 0; i < page_size; i++) {
			w_test_buffer[i] = rand_test();
			r_test_buffer[i] = w_test_buffer[i];
		}

		for (j = 0; j < 1; j++) {
			gxflash_pageprogram(block_addr + page_size * j, w_test_buffer, page_size);
			memset(oob_buffer, 0x00, oob_size);
			gxflash_readoob(block_addr + page_size*j, oob_buffer, oob_size);
		}

		reverse_bits(w_test_buffer, 0, test_bits);

		memset(page_buffer, 0xff, page_size+oob_size);
		memcpy(page_buffer, w_test_buffer, page_size);
		memcpy(page_buffer+page_size, oob_buffer, oob_size);

		for (j = 1; j < 64; j++) {
			gxflash_pageprogram_noecc(block_addr + page_size * j, w_test_buffer, page_size);
			gxflash_writeoob(block_addr + page_size*j, oob_buffer, oob_size);
		}

		ecc_corrected = 0;
		for (j = 1; j < 64; j++) {
			memset(w_test_buffer, 0xFF, page_size);
			gxflash_readdata(block_addr + page_size * j, w_test_buffer, page_size);
			bitflips = bit_diff(r_test_buffer, w_test_buffer, page_size);

			if (bitflips == 0)
				ecc_corrected = 1;
		}

		if (ecc_corrected == 0) {
			printf("%d, test bitflips %d, random data page bitflips %d\n", __LINE__, test_bits, bitflips);
			goto EXIT;
		}
	}

EXIT:
	if (test_bits == 0)
		printf("bitflips test failed\n");
	else if (test_bits == 1)
		printf("ECC test fail, the ECC may not be turn off\n", test_bits - 1);
	else
		printf("bitflips correct strength is %d\n", test_bits - 1);


	free(r_test_buffer);
	free(w_test_buffer);
	free(oob_buffer);
	free(page_buffer);
	return 0;
}

static uint32_t flash_get_fw_size(void)
{
	extern int _stage2_start_;
	extern int _stage2_data_end_;
	unsigned int data, page_size, block_size;
	unsigned int size = 0;

	data = gxflash_get_info(GX_FLASH_CHIP_TYPE);
	page_size = gxflash_get_info(GX_FLASH_PAGE_SIZE);
	block_size = gxflash_get_info(GX_FLASH_BLOCK_SIZE);

	if(data == GX_FLASH_CHIP_TYPE_NOR) {
		size = ROM_COPY_SIZE * 1 + 32 + (uint32_t)&_stage2_data_end_ - (uint32_t)&_stage2_start_;
	}else {
#ifdef CONFIG_ARCH_ARM_SIRIUS
		size = ROM_COPY_SIZE / 2048 * page_size * 5 + 32 + (uint32_t)&_stage2_data_end_ - (uint32_t)&_stage2_start_;
#elif defined(CONFIG_ARCH_ARM_CANOPUS)
		size = ROM_COPY_SIZE / 1024 * page_size * 5 + 32 + (uint32_t)&_stage2_data_end_ - (uint32_t)&_stage2_start_;
#elif defined(CONFIG_ARCH_ARM_VEGA)
		size = 8 * block_size + (uint32_t)&_stage2_data_end_ - (uint32_t)&_stage2_start_;
#else
		size = ROM_COPY_SIZE * 5 + 32 + (uint32_t)&_stage2_data_end_ - (uint32_t)&_stage2_start_;
#endif
	}
	return size;
}

/*
 * 用途：掉电测试准备
 * 原理：将测试代码所在区域以外的block填充随机数据
 * 总结：此命令可以用于NOR Flash、串行NAND和并行NAND，掉电测试需要继电器和上位机配合
 */
void flash_power_cut_test_init(void)
{
	uint32_t flash_size, erase_size, erase_num, i, j, fw_size, reserve_size, reserve_block_num;
	uint8_t *wb, *rb;
	unsigned int data;
	int ret;

	flash_size = gxflash_get_info(GX_FLASH_CHIP_SIZE);
	erase_size = gxflash_get_info(GX_FLASH_ERASE_SIZE);
	erase_num  = gxflash_get_info(GX_FLASH_ERASE_NUM);

	fw_size = flash_get_fw_size();
	reserve_block_num = (fw_size + erase_size + 1) / erase_size;
	reserve_size = reserve_block_num * erase_size;

	data = gxflash_get_info(GX_FLASH_CHIP_TYPE);
	printf("flash type: %s \n", data == GX_FLASH_CHIP_TYPE_NOR ? "spi nor":
			data == GX_FLASH_CHIP_TYPE_SPINAND ? "spi nand": "nand" );
	printf("flash size: 0x%x\n", flash_size);

	printf("firmware size: %d, reserve_size: 0x%x\n", fw_size, reserve_size);

	wb = malloc(erase_size);
	if (wb == NULL) {
		printf("malloc buffer failed\n");
	}
	rb = malloc(erase_size);
	if (rb == NULL) {
		printf("malloc buffer failed\n");
	}

	holdrand_test  = 1;

	for (i = 0; i < erase_size; i++)
		wb[i] = rand_test();

	for (i = reserve_block_num; i < erase_num; i++) {
		if (gxflash_block_isbad(erase_size * i) != 1) {
			gxflash_readdata(i * erase_size, rb, erase_size);
			for (j = 0; j < erase_size; j++) {
				if (wb[j] != rb[j]) {
					printf("program block %d\n", i);
					gxflash_erasedata(i * erase_size, erase_size);
					gxflash_pageprogram(i * erase_size, wb, erase_size);
					gxflash_readdata(i * erase_size, rb, erase_size);
				}
			}
		}
	}

	free(wb);
	free(rb);

	printf("power cut init OK\n");
}

/*
 * 用途：掉电测试
 * 原理：测试代码将检查所有块，并计算所有出现错误块数量，超过1个则判断测试失败
 * 总结：此命令可以用于NOR Flash、串行NAND和并行NAND，掉电测试需要继电器和上位机配合
 */
uint8_t *powercut_wb = NULL;
void flash_power_cut_test(void)
{
	uint32_t flash_size, erase_size, erase_num, i, j, k, test_block, fw_size, reserve_size, reserve_block_num;
	uint8_t *rb, *wb;
	int ret;
	int wrong_block_num = 0;
	int last_wrong_block;

	flash_size = gxflash_get_info(GX_FLASH_CHIP_SIZE);
	erase_size = gxflash_get_info(GX_FLASH_ERASE_SIZE);
	erase_num  = gxflash_get_info(GX_FLASH_ERASE_NUM);

	fw_size = flash_get_fw_size();
	reserve_block_num = (fw_size + erase_size + 1) / erase_size;
	reserve_size = reserve_block_num * erase_size;

	printf("firmware size: %d, reserve_size: 0x%x\n", fw_size, reserve_size);

	if (powercut_wb == NULL) {
		wb = malloc(erase_size);
		if (wb == NULL) {
			printf("malloc buffer failed\n");
			return;
		}
		powercut_wb = wb;
		holdrand_test  = 1;
		for (i = 0; i < erase_size; i++)
			wb[i] = rand_test();
	} else
		wb = powercut_wb;

	rb = malloc(erase_size);
	if (rb == NULL) {
		printf("malloc buffer failed\n");
		return;
	}

	last_wrong_block = -1;
	for (i = reserve_block_num; i < erase_num; i++) {
		if (gxflash_block_isbad(erase_size * i) != 1) {
			gxflash_readdata(i * erase_size, rb, erase_size);

			for (j = 0; j < erase_size; j++) {
				if (wb[j] != rb[j]) {
					wrong_block_num++;
					last_wrong_block = i;
					printf("read wrong data @block %d\n", i);
					printf("error data:0x%x, source data:0x%x, in flash offset:0x%x\n", rb[j], wb[j], i * erase_size + j);
					break;
				}
			}
		}
	}

	if (wrong_block_num > 1) {
		printf("wrong data block num: %d\n", wrong_block_num);
		goto EXIT;
	}

	if (last_wrong_block != -1) {
		gxflash_erasedata(last_wrong_block * erase_size, erase_size);
		gxflash_pageprogram(last_wrong_block * erase_size, wb, erase_size);
		gxflash_readdata(last_wrong_block * erase_size, rb, erase_size);

		for (j = 0; j < erase_size; j++) {
			if (wb[j] != rb[j]) {
				printf("read wrong data @block %d after program\n", i);
				printf("error data:0x%x, source data:0x%x, in flash offset:0x%x\n", rb[j], wb[j], i * erase_size + j);
				goto EXIT;
			}
		}
		printf("recovery block %d\n", last_wrong_block);
	}
	holdrand_test = flash_random_seed;
	printf("start random block test\n");
	for (k = 0; k < 30; k++) {
		test_block = rand_test() % (erase_num - reserve_block_num) + reserve_block_num;
		if (gxflash_block_isbad(erase_size * test_block) != 1) {
			printf("@@test block %d\n", test_block);
			gxflash_erasedata(test_block * erase_size, erase_size);
			gxflash_pageprogram(test_block * erase_size, wb, erase_size);
		}
	}

EXIT:
	printf("test exit\n");
	free(wb);
	free(rb);
}

void flash_nor_capacity_test(void)
{
	unsigned int flash_size, erase_size, erase_num, i, tmp;
	int ret;

	flash_size = gxflash_get_info(GX_FLASH_CHIP_SIZE);
	erase_size = gxflash_get_info(GX_FLASH_ERASE_SIZE);
	erase_num  = gxflash_get_info(GX_FLASH_ERASE_NUM);

	printf("flash size: 0x%x, erase size: 0x%x\n", flash_size, erase_size);

	printf("erase all flash, wait...\n");
	gxflash_erasedata(0, flash_size);

	for (i = 0; i < erase_num; i++) {
		ret = gxflash_pageprogram(erase_size * i, (unsigned char*)&i, sizeof(i));
		if (ret != 0) {
			printf("program fail at 0x%x, code = %d\n", erase_size * i, ret);
			goto OUTSIDE;
		}
	}

	printf("check flash all erase block mark\n");
	for (i = 0; i < erase_num; i++) {
		ret = gxflash_readdata(erase_size * i, (unsigned char*)&tmp, sizeof(tmp));
		if (ret != 0) {
			printf("read fail at 0x%x, code = %d\n", erase_size * i, ret);
			goto OUTSIDE;
		}

		if (tmp != i) {
			printf("block mark check fail at 0x%x\n", erase_size * i);
			goto OUTSIDE;
		}
	}

	printf("check flash capacity OK\n");

OUTSIDE:
	printf("read flash outside area test\n");
	ret = gxflash_readdata(flash_size, (unsigned char*)&tmp, sizeof(tmp));
	if (ret == 0)
		printf("read flash fail, code = %d\n", ret);
	else
		printf("read flash operation work ok\n");

	printf("erase flash outside area test\n");
	gxflash_erasedata(flash_size, erase_size);

	printf("program flash outside area test\n");
	ret = gxflash_pageprogram(flash_size, (unsigned char*)&flash_size, sizeof(flash_size));
	if (ret == 0)
		printf("program flash fail, code = %d\n", ret);
	else
		printf("program flash operation work ok\n");
}

void flash_capacity_test(void)
{
	flash_nor_capacity_test();
}

const static sflash_test_struct_t multi_program_test_array[] = {
	{0x1, 0x10}, {0x11, 0x10}, {0x2000, 0x100}, {0x20, 0x30}
};
int flash_nor_page_mutil_program_test(int offs, int size)
{
	uint8_t *mem, *mirror_mem;
	uint8_t random_data;
	uint32_t start, len, i, j, fixed_num, addr;
	int ret = 0;

	fixed_num = sizeof(multi_program_test_array) / sizeof(sflash_test_struct_t);

	printf("test offs 0x%x, size 0x%x\n", offs, size);
	mem = malloc(size);
	if (mem == NULL) {
		printf("%s: %d, malloc memory fail\n", __func__, __LINE__);
		return -1;
	}

	mirror_mem = malloc(size);
	if (mirror_mem == NULL) {
		printf("%s: %d, malloc memory fail\n", __func__, __LINE__);
		free(mem);
		return -1;
	}

	printf("erase size 0x%x, wait...\n", size);
	gxflash_erasedata(offs, size);
	memset(mirror_mem, 0xFF, size);

	for (j = 0; j < 3 + fixed_num; j++) {
		if (j < fixed_num) {
			start = multi_program_test_array[j].addr;
			len   = multi_program_test_array[j].len;

			if (start + len > size)
				continue;
		} else {
			start = rand_test() % size;
			len   = rand_test() % (size - start);
		}

		for (i = 0; i < len; i++) {
			random_data          = rand_test();
			mem[i]               = random_data;
			mirror_mem[start+i] &= random_data;
		}

		addr = offs + start;

		printf("program offs 0x%x, len 0x%x\n", addr, len);

		gxflash_pageprogram(addr, mem, len);

		memset(mem, 0x00, size);
		gxflash_readdata(offs, mem, size);

		for (i = 0; i < size; i++) {
			if (mem[i] != mirror_mem[i]) {
				printf("compare random data fail at offset:0x%x, cmp data: 0x%x, 0x%x\n", i, mem[i], mirror_mem[i]);
				ret = -1;
				goto EXIT;
			}
		}
	}

	printf("multi program test OK!\n");

	/* show final data
	for (i = 0; i < size; i++) {
		printf("%02x", mem[i]);
	}
	*/

EXIT:
	free(mem);
	free(mirror_mem);

	return ret;
}

void flash_multi_proram_test(void)
{
	unsigned int flash_size, erase_size, page_size, test_size;
	int offs, size, i, ret;
	unsigned int data;

	data = gxflash_get_info(GX_FLASH_CHIP_TYPE);

	if (data != GX_FLASH_CHIP_TYPE_NOR) {
		printf("only support NOR Flash, exit\n");
		return;
	}

	flash_size = gxflash_get_info(GX_FLASH_CHIP_SIZE);
	erase_size = gxflash_get_info(GX_FLASH_ERASE_SIZE);
	page_size  = gxflash_get_info(GX_FLASH_PAGE_SIZE);

	printf("flash size 0x%x, erase size 0x%x, page size 0x%x\n", flash_size, erase_size, page_size);

	flash_nor_page_mutil_program_test(0, page_size);
	flash_nor_page_mutil_program_test(0, erase_size);
	flash_nor_page_mutil_program_test(flash_size - page_size, page_size);
	flash_nor_page_mutil_program_test(flash_size - erase_size, erase_size);

	for (i = 0; i < 100; i++) {
		offs = rand_test() % flash_size;
		size = rand_test() % (flash_size - offs);

		ret = flash_nor_page_mutil_program_test(offs, size);
		if (ret != 0)
			break;
	}
}

void flash_otp_test(int page)
{
	unsigned char *writebuf, *readbuf;
	unsigned char status;
	int ret, otp_size, start_page, end_page;
	unsigned int region_num = 0, i, j;

	ret = gxflash_otp_get_region_size(&otp_size);

	if (ret != 0) {
		printf("not support otp\n");
		return;
	}

	printf("otp per region size: %d\n", otp_size);

	writebuf = malloc(otp_size);
	if (writebuf == NULL) {
		printf("malloc writebuf fail\n");
		goto exit;
	}

	readbuf = malloc(otp_size);
	if (readbuf == NULL) {
		printf("malloc readbuf fail\n");
		goto exit;
	}

	for (j = 0; j < otp_size; j++) {
		writebuf[j] = rand_test();
	}

	gxflash_otp_get_region(&region_num);

	if (region_num == 0) {
		printf("no otp region\n");
		goto exit;
	}

	printf("otp contain %d region, will test otp erase, read, write\n", region_num);

	if (page == -1) {
		start_page = 0;
		end_page = region_num - 1;
	} else {
		start_page = page;
		end_page = page;
	}

	for (i = start_page; i <= end_page; i++) {
		ret = gxflash_otp_set_region(i);

		if (ret != 0) {
			printf("set otp region fail, err = %d\n", ret);
			break;
		}

		gxflash_otp_status(&status);
		if (status == 0) {
			printf("otp region %d unlock\n", i);
		} else {
			printf("otp region %d lock\n", i);
		}

		printf("    erase otp region %d\n", i);
		gxflash_otp_erase();
		memset(readbuf, 0xab, otp_size);
		printf("    read otp region %d\n", i);
		gxflash_otp_read(0, readbuf, otp_size);
		for (j = 0; j < otp_size; j++) {
			if (readbuf[j] != 0xff) {
				printf("    read otp offset readbuf[%d] not 0xff\n", j);
				goto exit;
			}
		}

		printf("    write otp region %d\n", i);
		gxflash_otp_write(0, writebuf, otp_size);
		memset(readbuf, 0xab, otp_size);
		printf("    read otp region %d\n", i);
		gxflash_otp_read(0, readbuf, otp_size);
		for (j = 0; j < otp_size; j++) {
			if (readbuf[j] != writebuf[j]) {
				printf("    read otp offset readbuf[%d] != 0x%02x\n", j, writebuf[j]);
				goto exit;
			}
		}
		printf("    otp region %d test finish\n", i);
	}

exit:
	if (writebuf != NULL)
		free(writebuf);

	if (readbuf != NULL)
		free(readbuf);
}

void flash_otp_test_status(void)
{
	unsigned char status;
	int ret, otp_size, start_page, end_page, current_page;
	unsigned int region_num = 0, i, j;

	ret = gxflash_otp_get_region_size(&otp_size);

	if (ret != 0) {
		printf("not support otp\n");
		return;
	}

	printf("otp per region size: %d\n", otp_size);

	gxflash_otp_get_region(&region_num);

	if (region_num == 0) {
		printf("no otp region\n");
		return;
	}

	printf("otp contain %d region\n", region_num);

	gxflash_otp_get_current_region(&current_page);
	printf("otp current at region %d\n", current_page);

	for (i = 0; i < region_num; i++) {
		ret = gxflash_otp_set_region(i);

		if (ret != 0) {
			printf("set otp region fail, err = %d\n", ret);
			break;
		}

		gxflash_otp_status(&status);
		if (status == 0) {
			printf("otp region %d unlock\n", i);
		} else {
			printf("otp region %d lock\n", i);
		}
	}

	gxflash_otp_set_region(current_page);
}


void flash_otp_write_page(int page)
{
	unsigned char *writebuf;
	unsigned char status;
	int i, size;

	gxflash_otp_set_region(page);
	gxflash_otp_get_current_region(&page);
	gxflash_otp_get_region_size(&size);
	printf("otp region %d size: 0x%x\n", page, size);

	writebuf = malloc(size);
	if (writebuf == NULL) {
		printf("malloc writebuf fail\n");
		return;
	}

	for (i = 0; i < size; i++) {
		writebuf[i] = rand_test();
	}

	gxflash_otp_status(&status);
	if (status == 0) {
		printf("otp region %d unlock\n", page);
	} else {
		printf("otp region %d lock\n", page);
	}

	printf("write otp region %d\n", page);
	gxflash_otp_write(0, writebuf, size);

	free(writebuf);
}

void flash_otp_read_page(int page)
{
	unsigned char *readbuf;
	unsigned char status;
	int i, size;

	gxflash_otp_set_region(page);
	gxflash_otp_get_current_region(&page);
	gxflash_otp_get_region_size(&size);
	printf("otp region %d size: 0x%x\n", page, size);

	readbuf = malloc(size);
	if (readbuf == NULL) {
		printf("malloc readbuf fail\n");
		return;
	}

	for (i = 0; i < size; i++) {
		readbuf[i] = rand_test();
	}

	memset(readbuf, 0x00, size);
	printf("read otp region %d\n", page);
	gxflash_otp_read(0, readbuf, size);
	for (i = 0; i < size; i++) {
		printf("%02x ", readbuf[i]);
	}

	printf("\n");

	free(readbuf);
}

void flash_speed_test(void)
{
	unsigned long start_time, end_time, i;
	uint8_t *p1, *p2;
	uint32_t size;
	unsigned int data;
	uint32_t speed;

	data = gxflash_get_info(GX_FLASH_CHIP_TYPE);
	if (data == GX_FLASH_CHIP_TYPE_NOR)
		size = gxflash_get_info(GX_FLASH_CHIP_SIZE);
	else
		size = 0x800000;

	gxflash_write_protect_unlock();

	p1 = malloc(size);
	p2 = malloc(size);

	if (p1 == NULL) {
		printf("malloc fail\n", __func__, __LINE__);
		return;
	}

	if (p2== NULL) {
		printf("malloc fail\n", __func__, __LINE__);
		free(p1);
		return;
	}

	for (i = 0; i < size; i++) {
		p1[i] = i * 17;
	}

	printf("erase flash size %dMiB\n", size >> 20);

	start_time = gx_time_get_ms();
	gxflash_erasedata(0, size);
	end_time = gx_time_get_ms();

	start_time = gx_time_get_ms();
	gxflash_pageprogram(0, p1, size);
	end_time = gx_time_get_ms();
	speed =  size / 1024 * 1000 / (end_time - start_time);
	printf("program %dMiB elapse %dms, %u KB/S\n", size >> 20, end_time - start_time, speed);

	start_time = gx_time_get_ms();
	gxflash_readdata(0, p2, size);
	end_time = gx_time_get_ms();
	speed =  size / 1024 * 1000 / (end_time - start_time);
	printf("read %dMiB elapse %dms, %u KB/S\n", size >> 20, end_time - start_time, speed);

	start_time = gx_time_get_ms();
	gxflash_erasedata(0, size);
	end_time = gx_time_get_ms();
	speed =  size / 1024 * 1000 / (end_time - start_time);
	printf("erase %dMiB elapse %dms, %u KB/S\n", size >> 20, end_time - start_time, speed);

	free(p1);
	free(p2);
}

static uint8_t buf[16][16];
void flash_uid_test(void)
{
	int retlen, i, j;
	int ret;

	ret = gxflash_uid_read((unsigned char*)buf, sizeof(buf), &retlen);

	if (ret != 0) {
		printf("ret = %d\n", ret);
		return;
	}


	printf("uid: \n");

	for (j = 0; j < sizeof(buf) / 16; j++) {
		for (i = 0; i < 16; i++) {
			printf("%02x ", buf[j][i]);
		}
		printf("\n");
	}

	printf("\n");
}

void flash_all0(void)
{
	unsigned int flash_size, erase_size, erase_num, i, j, tmp;
	int ret;
#define BUF_LENGTH (128 * 1024)

	flash_size = gxflash_get_info(GX_FLASH_CHIP_SIZE);
	erase_size = gxflash_get_info(GX_FLASH_ERASE_SIZE);
	erase_num  = gxflash_get_info(GX_FLASH_ERASE_NUM);
	char* readbuf = malloc(BUF_LENGTH );

	printf("flash size: 0x%x, erase size: 0x%x\n", flash_size, erase_size);

	//printf("erase all flash, wait...\n");
	// gxflash_erasedata(0, flash_size);

	for (i = 0; i < erase_num; i++) {
		gxflash_erasedata(erase_size * i, erase_size);
		memset(readbuf, 0, BUF_LENGTH);
		ret = gxflash_pageprogram(erase_size * i, (unsigned char*)readbuf, BUF_LENGTH );
		if (ret != 0) {
			printf("program fail at 0x%x, code = %d\n", erase_size * i, ret);
			goto OUTSIDE;
		}

		memset(readbuf, 0xaa, BUF_LENGTH);
		gxflash_readdata(erase_size * i, (unsigned char*)readbuf, BUF_LENGTH);

		for (j = 0; j < BUF_LENGTH; j++) {
			if (readbuf[j] != 0x00) {
				printf("[0x%02x] != 0x00\n");
			}

		}
	}
OUTSIDE:
	free(readbuf);
}

void flash_info_test(void)
{
	unsigned int data;
	data = gxflash_get_info(GX_FLASH_CHIP_TYPE);
	printf("flash type: %s \n", data == GX_FLASH_CHIP_TYPE_NOR ? "spi nor":
			data == GX_FLASH_CHIP_TYPE_SPINAND ? "spi nand": "nand" );
	data = gxflash_get_info(GX_FLASH_CHIP_NAME);
	printf("flash name: %s \n", data);
	data = gxflash_get_info(GX_FLASH_CHIP_ID);
	printf("flash id: 0x%x \n", data);
	data = gxflash_get_info(GX_FLASH_CHIP_SIZE);
	printf("flash chip size: %d M\n", data/1024/1024);
	data = gxflash_get_info(GX_FLASH_BLOCK_SIZE);
	printf("flash block size: %d K\n", data/1024);
	data = gxflash_get_info(GX_FLASH_BLOCK_NUM);
	printf("flash block num: %d \n", data);
	data = gxflash_get_info(GX_FLASH_SECTOR_SIZE);
	printf("flash sector size: %d K\n", data/1024);
	data = gxflash_get_info(GX_FLASH_SECTOR_NUM);
	printf("flash sector num: %d \n", data);
	data = gxflash_get_info(GX_FLASH_PAGE_SIZE);
	printf("flash page size: %d B\n", data);
	data = gxflash_get_info(GX_FLASH_PAGE_NUM);
	printf("flash page num: %d \n", data);
	data = gxflash_get_info(GX_FLASH_ERASE_SIZE);
	printf("flash erase size: %d K\n", data/1024);
	data = gxflash_get_info(GX_FLASH_ERASE_NUM);
	printf("flash erase num: %d \n", data);
	data = gxflash_get_info(GX_FLASH_PROTECT_MODE);
	printf("flash protect mode : %s \n", data == GX_FLASH_PROTECT_MODE_BOTTOM ? "bottom":
			data == GX_FLASH_PROTECT_MODE_TOP ? "top": "nonsupport" );
	data = gxflash_get_info(GX_FLASH_MATCH_MODE);
	printf("flash match mode : %s \n", data == GX_FLASH_MATCH_MODE_ID ? "id":
			data == GX_FLASH_MATCH_MODE_BLUR ? "blur": "nonsupport" );
	data = gxflash_get_info(GX_FLASH_PROTECT_LEN);
	printf("flash protect len: 0x%x \n", data);
	if(gxflash_get_info(GX_FLASH_CHIP_TYPE) != GX_FLASH_CHIP_TYPE_NOR) {
		data = gxflash_get_info(GX_FLASH_OOB_SIZE);
		printf("flash oob size: 0x%x \n", data);
	}
}

#if (defined(CONFIG_ENABLE_DWSPI) && defined(CONFIG_ENABLE_SFLASH)) || (defined(CONFIG_ENABLE_DWSPI) && defined(CONFIG_ENABLE_SPINAND))
extern int dw_spi_config(unsigned int clk_div, unsigned int sample_delay, unsigned short mode);
int flash_sample_delay_test(uint32_t div)
{
	uint32_t sample_delay = 0;
	uint32_t buf[256] = {0};
	uint8_t i = 0, j = 0, start = 0, end = 0;

	sample_delay_test = 1;

	while(1) {
		if(end == 1 && j > 3) break;

		printf("sample_delay test is %u\n", sample_delay);

		if (dw_spi_config(div, sample_delay, SPI_TX_DUAL | SPI_RX_DUAL) < 0) {
			printf("spi config failed.\n");
			return -1;
		}

		if(flash_complete_test(100)) {
			printf("sample_delay %u is test failed!\n", sample_delay);
			if(start == 1 && end == 0) end = 1;
			if(start == 1) j++;
			sample_delay++;
			continue;
		}

		if (dw_spi_config(div, sample_delay, SPI_TX_QUAD | SPI_RX_QUAD) < 0) {
			printf("spi config failed.\n");
			return -1;
		}

		if(flash_complete_test(100)) {
			printf("sample_delay %u is test failed!\n", sample_delay);
			if(start == 1 && end == 0) end = 1;
			if(start == 1) j++;
			sample_delay++;
			continue;
		}

		if(start == 0 && end == 0) start = 1;
		printf("sample_delay %u is test ok!\n", sample_delay);
		buf[i] = sample_delay;
		i++;
		sample_delay++;
	}

	printf("==============================\n");
	printf("sample_delay correct number is ");
	for(j = 0; j < i; j++)
		printf("%u, ", buf[j]);
	printf("\n");
	if(i == 0)
		printf("sample_delay test no number!!!\n");
	else {
		int cnt = i % 2 ? (i/2) : i/2 - 1;
		sample_delay = buf[cnt];
		dw_spi_config(div, sample_delay, SPI_TX_DUAL | SPI_RX_DUAL);
		printf("sample_delay the best number is %u\n", sample_delay);
	}
	printf("==============================\n");

	sample_delay_test = 0;

	return 0;
}
#endif

int gxflash_exec_testcase(void)
{
	int i =0, ret = 0;
	unsigned int region_num = 0;

	printf("************************erase read write test.************************\n");
	flash_complete_test(1000);
	printf("**************************multiprogram test.**************************\n");
	flash_multi_proram_test();
	printf("****************************WP test.**********************************\n");
	flash_wp_test();
	printf("****************************OTP test.*********************************\n");
	flash_otp_test(-1);
	printf("****************************OTP lock test.*********************************\n");
	gxflash_otp_get_region(&region_num);
	for (i = 0; i < region_num; i++) {
		ret = gxflash_otp_set_region(i);
		if (ret != 0) {
			printf("set otp region fail, err = %d\n", ret);
			break;
		}
		gxflash_otp_lock();
	}
	flash_otp_test(-1);
}

int flash_unalign_test_single(uint32_t addr, uint32_t len, uint32_t buf_align)
{
	uint8_t *w_buf_base = NULL, *r_buf_base = NULL;
	uint8_t *w_buf = NULL, *r_buf = NULL;
	int i, ret = 0;
	uint32_t alloc_size;

	alloc_size = len + buf_align;

	w_buf_base = malloc(alloc_size);
	r_buf_base = malloc(alloc_size);
	if (!w_buf_base || !r_buf_base) {
		printf("malloc failed\n");
		ret = -1;
		goto out;
	}

	w_buf = (uint8_t *)((unsigned long)w_buf_base + buf_align);
	r_buf = (uint8_t *)((unsigned long)r_buf_base + buf_align);

	printf("w_buf: 0x%x, r_buf: 0x%x\n", (unsigned int)w_buf, (unsigned int)r_buf);

	for (i = 0; i < len; i++) {
		w_buf[i] = rand_test();
	}

	gxflash_readdata(addr, r_buf, len);
	printf("unalign_test read finish.\n");

	gxflash_erasedata(addr, len);
	printf("unalign_test erase finish.\n");

	gxflash_readdata(addr, r_buf, len);
	printf("unalign_test read finish.\n");
	for (i = 0; i < len; i++) {
		if (r_buf[i] != 0xFF) {
			printf("Erase check failed at offset %d: expected 0xFF, got 0x%02x\n",
			       i, r_buf[i]);
			ret = -1;
			goto out;
		}
	}

	gxflash_pageprogram(addr, w_buf, len);
	printf("unalign_test pageprogram finish.\n");

	memset(r_buf, 0, len);
	gxflash_readdata(addr, r_buf, len);
	printf("unalign_test read finish.\n");

	for (i = 0; i < len; i++) {
		if (r_buf[i] != w_buf[i]) {
			printf("Compare failed at offset %d: expected 0x%02x, got 0x%02x\n",
			       i, w_buf[i], r_buf[i]);
			ret = -1;
			goto out;
		}
	}

	printf("cmp finish. src equal dst.\n\n");

out:
	if (w_buf_base)
		free(w_buf_base);
	if (r_buf_base)
		free(r_buf_base);
	return ret;
}

void flash_unalign_complete_test(unsigned int count)
{
	uint32_t flash_size, erase_size, erase_num;
	uint32_t align;
	int j;
	unsigned int data;

	flash_size = gxflash_get_info(GX_FLASH_CHIP_SIZE);
	erase_size = gxflash_get_info(GX_FLASH_ERASE_SIZE);
	erase_num = gxflash_get_info(GX_FLASH_ERASE_NUM);
	data = gxflash_get_info(GX_FLASH_CHIP_TYPE);

	gxflash_write_protect_unlock();

	for (align = 1; align <= 64; align++) {
		printf("\nunalign_test buffer alignment %d bytes\n\n", align);

		j = 0;
		while (j < count) {
			uint32_t addr_tmp, len_tmp;

			addr_tmp = flash_size * ((float)rand_test()/(0x7fff+1)) + rand_test();
			len_tmp = RAND_TEST_LEN_MAX * ((float)rand_test()/(0x7fff+1)) + rand_test();

			if (data == GX_FLASH_CHIP_TYPE_NOR) {
				if (addr_tmp + len_tmp > flash_size)
					continue;
			} else {
				uint32_t addr_offs = 0x1000 * ((float)rand_test()/(0x7fff+1));
				addr_tmp += addr_offs;
				if (addr_tmp + len_tmp > flash_size - erase_size * erase_num * 2 / 100)
					continue;
			}

			printf(">>>>>unalign_test:addr=0x%x, len = 0x%x\n", addr_tmp, len_tmp);
			if (flash_unalign_test_single(addr_tmp, len_tmp, align) != 0) {
				printf("unalign_test failed at addr=0x%x len=0x%x align=%d\n", addr_tmp, len_tmp, align);
				while (1);
			}
			j++;
		}
		printf(">>>>>unalign_test_success, align: %d.\n\n\n", align);
	}

	printf("\nunalign test finished!\n");
}

#endif

