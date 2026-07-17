#include "stdio.h"
#include "string.h"
#include "config.h"
#include <usb.h>
#include "zlib.h"
#include "boot.h"
#include "romfs.h"
#include <gx_flash_common.h>
#include <flash.h>
#include "gx_flash.h"
#include "irr.h"
#include "interrupt.h"
#include "load_kernel.h"
#include "gx_api.h"
#include "cpu/cpu.h"
#include "jpeg.h"
#include "misc.h"
#include "common/gx_mem_info.h"
#include <limits.h>
#ifdef CONFIG_ENABLE_OTA
#include "gxota.h"
#endif
#ifdef CONFIG_ENABLE_KERNEL_DTB
#include "fdt_support.h"
#include "libfdt.h"
#endif
#include "gxloader_log.h"

#ifdef CONFIG_ECOS_OTA
#include "common/gxmalloc.h"
#define page_malloc(size)      GxCore_Malloc(size)
#define page_free(size)        GxCore_Free(size)
#define partition_read(partition, offset, buf, size)   GxCore_PartitionRead(partition, buf, size, offset)
#endif

/******************************************************************
 *
 * rtc is mainly used for caculate the time of loader
 *
 ******************************************************************/
#ifdef CONFIG_ENABLE_TIME
#include <time.h>
extern unsigned long g_start_time;
extern unsigned long g_end_time;
#endif

#ifdef CONFIG_ENABLE_CTR
#include "ctr.h"
#endif

extern struct partition *flash_entry_arr[FLASH_TYPE_NUM];

#ifdef CONFIG_ENABLE_TEE
static unsigned int g_tee_load_addr = 0;
static unsigned int g_tee_entry_addr = 0;
#if defined(CONFIG_ENABLE_CMD) && CONFIG_BOOTDELAY > 0
static unsigned int g_ree_kernel_addr = 0;
static unsigned int g_ree_dtb_addr = 0;
#endif
#endif

#ifndef CONFIG_ECOS_OTA
static int boot_close_module(void)
{
	int ret = 0;
#ifdef CONFIG_ENABLE_CTR
	gx_ctr_disable();
#endif

#ifdef CONFIG_ENABLE_IRR
	irr_close();
#endif

#ifdef CONFIG_ENABLE_IRQ
	gx_disable_all_interrupt();
#endif

#if defined(CONFIG_ARCH_CKMMU)
	dcache_flush();
	icache_flush();
#elif defined(CONFIG_ARCH_ARM)
	dcache_flush();
	dcache_disable();
	mmu_disable();
	dcache_invalidate();
	icache_disable();
	icache_invalidate();
#else
	gxlogi("(%s %d)ERROR ARCH : %s\n", __func__, __LINE__, CONFIG_ARCH);
	ret = -1;
#endif

	return ret;
}

void mem_printf(char *buf, size_t mem) {
	char *fmt[] = {"%d", "%dk", "%dm"};
	int id = 0;

	if ((mem & 0xFFFFF) == 0) {
		id = 2;
		mem >>= 20;
	}
	else if ((mem & 0x3FF) == 0) {
		id = 1;
		mem >>= 10;
	}

	sprintf(buf, fmt[id], mem);
}

static void partition_info_cmdline(char *cmdline_addr) {
	int i;
	int num;
	struct partition *flash_entry;

	num = flash_get_type();
	flash_entry = flash_entry_arr[num];

	/* no partition */
	if(flash_entry->count == 0){
		return;
	}

#ifdef CONFIG_ENABLE_SFLASH
	sprintf(cmdline_addr, " mtdparts=m25p80:");
#elif (defined(CONFIG_ENABLE_NANDFLASH) || defined(CONFIG_ENABLE_SPINAND))
	sprintf(cmdline_addr, " mtdparts=gx320x:");
#endif
	for(i = 0; i < flash_entry->count; i++){
		if (flash_entry->tables[i].file_system_type == HIDE)
			continue;
		mem_printf(cmdline_addr + strlen(cmdline_addr), flash_entry->tables[i].total_size);
		strcat(cmdline_addr, "@");
		mem_printf(cmdline_addr + strlen(cmdline_addr), flash_entry->tables[i].start_addr);

		sprintf(cmdline_addr + strlen(cmdline_addr), "(%s),", flash_entry->tables[i].name);
	}

	cmdline_addr[strlen(cmdline_addr) -1 ] = '\0';
}

void setup_start_tag(struct tag *params)
{
	params->hdr.tag = ATAG_CORE;
	params->hdr.size = tag_size(tag_core);
	params->u.core.flags = 0;
	params->u.core.pagesize = 0;
	params->u.core.rootdev = 0;
	params = tag_next(params);
}

void setup_memory_tags (struct tag *params)
{
	params->hdr.tag = ATAG_MEM;
	params->hdr.size = tag_size(tag_mem32);
	params->u.mem.start = DRAMBASE;
	params->u.mem.size = DRAM_SIZE;
	params = tag_next(params);
}

void setup_commandline_tag(struct tag *params, char *commandline)
{
	char *p;

	if (!commandline)
		return;

	for (p = commandline; *p == ' '; p++) ;

	if (*p == '\0')
		return;

	params->hdr.tag = ATAG_CMDLINE;
	params->hdr.size = (sizeof(struct tag_header) + strlen(p) + 1 + 4) >> 2;
	strcpy(params->u.cmdline.cmdline, p);
	params = tag_next(params);
}

void setup_end_tag(struct tag *params)
{
	params->hdr.tag  = ATAG_NONE;
	params->hdr.size = 0;
}

void rtc_printf(void)
{
	g_end_time = gx_time_get_ms();
	gxlogi("The total boot time is: %lu s (%lu ms)\r\n", (g_end_time-g_start_time)/1000, (g_end_time-g_start_time));
}

/*
 * Run Linux & Ecos
 */
unsigned long memparse(char *ptr, char **retptr)
{
        char *endptr;   /* local pointer to end of parsed string */

        unsigned long ret=0;
        ret = simple_strtoull(ptr, &endptr, 0);

        switch (*endptr) {
        case 'G':
        case 'g':
                ret <<= 10;
        case 'M':
        case 'm':
                ret <<= 10;
        case 'K':
        case 'k':
                ret <<= 10;
                endptr++;
        default:
                break;
        }

        if (retptr)
                *retptr = endptr;

        return ret;
}

static unsigned int bootstr_cmdline[COMMAND_LINE_SIZE/4 + OTHER_TAG_SIZE] = {0}; // use u32 to ensure align 4

char * get_cmdline(void)
{
	struct tag *params = (struct tag*)bootstr_cmdline;

	return (char*)&params->u.cmdline;
}

unsigned long cmdline_get_boot_params(void)
{
	return (unsigned long)bootstr_cmdline;
}

void cmdline_add(const char *cmd)
{
	struct tag *params = (struct tag*)bootstr_cmdline;
	strcat(params->u.cmdline.cmdline, cmd);
	strcat(params->u.cmdline.cmdline, " ");
}

void cmdline_add_noappend_space(const char *cmd)
{
	struct tag *params = (struct tag*)bootstr_cmdline;
	strcat(params->u.cmdline.cmdline, cmd);
}

static int cmdline_protect_flag_add(uint32_t value)
{
#define LEN	(256)
	char *flag = malloc(LEN);
	if (!flag) {
		gxlogi("error: %s %d\n", __func__, __LINE__);
		return -1;
	}
	memset(flag, 0x0, LEN);

	strcpy(flag, "protect_flag=");
	mem_printf(flag + strlen(flag), value);
	cmdline_add(flag);
	free(flag);

	return 0;
}

extern int protect_flag_probe(void);

void cmdline_init(const char *cmd)
{
	int protect_flag = 0;
	struct tag *params;
	params = (struct tag*)bootstr_cmdline;

	params->hdr.tag = 0x54410003; /* MAGIC */
	params->hdr.size = COMMAND_LINE_SIZE/4;

	cmdline_add(cmd);
	if ((protect_flag = protect_flag_probe()) != 0)
		cmdline_protect_flag_add(protect_flag);
	else if (CONFIG_MEMORY_PROTECT_TYPE)
		cmdline_protect_flag_add(CONFIG_MEMORY_PROTECT_TYPE);

#ifdef ENABLE_SECURE_ALIGN
	cmdline_add("secure_align");
#endif
	params = tag_next(params);
	params->hdr.tag = 0x0;
	params->hdr.size = 0;
	/* use for gdb debug to transfer cmdline */
	char *buf = (char *)bootstr_cmdline;
#ifdef CONFIG_ARCH_CKMMU
	// CK MMU
	asm volatile(
		"lrw	r7, %0\n\t"
		"mtcr	r7, ss2\n\t"
		"ld	r7, %1\n\t"
		"mtcr	r7, ss3\n\t"
		:
		:"i"(CMDLINE_MAGIC),"m"(buf)
		:"r7"
	);
#elif defined(CONFIG_ARCH_ARM)
	// r11 may not be used now, so temporary set cmdline addr in r11
	asm volatile (
		"ldr r11, %0;"
		:
		:"m"(buf)
		:
	);
#endif
}

void cmdline_clear(void)
{
	memset(bootstr_cmdline, 0x00, sizeof(bootstr_cmdline));
}

void update_cmdline(const char *cmd, struct partition_table_info *table_info)
{
	cmdline_init(cmd);
#ifdef CONFIG_ARCH_CKMMU_GX6605S
	cmdline_add("chipid=0x6605");
#endif
	all_partition_init(table_info);
}

void flash_partition_to_cmdline(struct partition *flash_entry, char *p_parts)
{
	int i;

	/* no partition */
	if(flash_entry->count == 0)
		return;

	for(i = 0; i < flash_entry->count; i++){
		if (flash_entry->tables[i].file_system_type == HIDE)
			continue;
		mem_printf(p_parts + strlen(p_parts), flash_entry->tables[i].total_size);
		strcat(p_parts, "@");
		mem_printf(p_parts + strlen(p_parts), flash_entry->tables[i].start_addr);

		sprintf(p_parts + strlen(p_parts), "(%s),", flash_entry->tables[i].name);
	}
	p_parts[strlen(p_parts) - 1] = ';';
}

static int get_used_partition_count(struct partition *flash_entry)
{
	int i;
	int count = 0;

	for (i = 0; i < flash_entry->count; i++) {
		if (flash_entry->tables[i].file_system_type != HIDE)
			count++;
	}

	return count;
}

/*
 * The order of rootfs mtd id is: nand flash, spi nor, spi nand.
 * It is determined by the order of flash device probing in kernel.
 */
static int get_rootfs_mtd_id(struct partition_info *p, int flash_type)
{
	int mtd_id = 0;
	char map[FLASH_TYPE_NUM];

	flash_get_used_device_map(map);

	if (flash_type == 0) { //spi nor
		if (map[2] == 1)
			mtd_id += get_used_partition_count(flash_entry_arr[2]);
	} else if (flash_type == 1) { //spi nand
		if (map[0] == 1)
			mtd_id += get_used_partition_count(flash_entry_arr[0]);
		if (map[2] == 1)
			mtd_id += get_used_partition_count(flash_entry_arr[2]);
	}

	mtd_id += p->mtd_id;

	return mtd_id;
}

static void mtdparts_clr_cmdline(void)
{
	char *cmdline = NULL;
	char *cmd_s = NULL;
	char *cmd_e = NULL;
	char *cmd_tmp = NULL;

	struct tag *params = (struct tag*)bootstr_cmdline;
	cmdline = (char*)&params->u.cmdline;
	char mtdparts_end[] = "mtdparts_end ";

	if ((cmd_s = strstr(cmdline, "m25p80_extend=1")) || \
				(cmd_s = strstr(cmdline, "mtdparts="))) {
		cmd_e = strstr(cmdline, mtdparts_end)+strlen(mtdparts_end);
		cmd_tmp = (char *)malloc(strlen(cmd_e)+1);
		// backup tail cmdline msg
		strcpy(cmd_tmp, cmd_e);
		// clear mtdparts cmdline msg
		memset(cmd_s, 0, strlen(cmd_s));
		// copy tail cmdline msg to head
		strcpy(cmd_s, cmd_tmp);
		free(cmd_tmp);
	}
}

void mtdparts_to_cmdline(void)
{
	int i;
	char *p_parts = NULL;
	struct tag *params = (struct tag*)bootstr_cmdline;
	char map[FLASH_TYPE_NUM];
	int rootfs_mtdid = 0;
	int flash_type;
	int type;
	char mtdparts_end[] = "mtdparts_end";
	type = flash_get_type();
	flash_get_used_device_map(map);

	mtdparts_clr_cmdline();

	/* if no root=, add automatically */
	if (strstr(params->u.cmdline.cmdline, "root=") == NULL) {
		int len = 42;
		char root[len];
		memset(root, 0, len);
		struct partition_info *p = all_partition_get("rootfs");
		if (p == NULL)
			p = all_partition_get("root");
		if (p) {
			flash_type = flash_get_type();
			rootfs_mtdid = get_rootfs_mtd_id(p, flash_type);
#ifdef CONFIG_ENABLE_ROOTFS_UBIFS
			sprintf(root, "ubi.mtd=%d", rootfs_mtdid);
			cmdline_add(root);
			sprintf(root, "root=ubi0_0");
			cmdline_add(root);
			sprintf(root, "rootfstype=%s", p->file_system_type ? get_fstype(p->file_system_type) : "ubifs");
			cmdline_add(root);
#elif defined(CONFIG_ENABLE_ROOTFS_INITRD)
			sprintf(root, "initrd=0x%x,0x%0x", INITRD_DRAM_START_ADDR, INITRD_DRAM_SIZE);
			cmdline_add(root);
			sprintf(root, "root=/dev/ram rw");
			cmdline_add(root);
#else
			sprintf(root, "root=/dev/mtdblock%d", rootfs_mtdid);
			cmdline_add(root);
#endif
		}
	}

	/* parse parts to cmdline */
	if ((p_parts = malloc(COMMAND_LINE_SIZE)) == NULL) {
		gxlogi("error: malloc fail! file: %s, line: %d\n", __FILE__, __LINE__);
		return;
	}

	memset(p_parts, 0, COMMAND_LINE_SIZE);
#ifdef CONFIG_EXTEND_MTDPARTS
	strcat(p_parts, "m25p80_extend=1 ");
#endif
	strcat(p_parts, "mtdparts=");

#ifdef CONFIG_ENABLE_ADAPTIVE_FLASH
	if (adaptive.flash_type > 0) {
		flash_switch_type(adaptive.flash_type - 1);
		if (adaptive.flash_type == FLASH_TYPE_SPI_NOR)
			strcat(p_parts, MTD_ID_SPI_NOR);
		else if (adaptive.flash_type == FLASH_TYPE_SPI_NAND)
			strcat(p_parts, MTD_ID_SPI_NAND);
		else if (adaptive.flash_type == FLASH_TYPE_NAND)
			strcat(p_parts, MTD_ID_NAND);
		flash_partition_to_cmdline(flash_entry_arr[adaptive.flash_type - 1], p_parts);
	}
#else
	if (map[0] == 1) {
		i = 0;
		flash_switch_type(i);
		strcat(p_parts, MTD_ID_SPI_NOR);
		flash_partition_to_cmdline(flash_entry_arr[i], p_parts);
	}
	if (map[1] == 1) {
		i = 1;
		flash_switch_type(i);
		strcat(p_parts, MTD_ID_SPI_NAND);
		flash_partition_to_cmdline(flash_entry_arr[i], p_parts);
	} else if (map[2] == 1) {
		i = 2;
		flash_switch_type(i);
		strcat(p_parts, MTD_ID_NAND);
		flash_partition_to_cmdline(flash_entry_arr[i], p_parts);
	}
#endif

#ifdef CONFIG_EXTEND_MTDPARTS
	strcat(p_parts, CONFIG_EXTEND_MTDPARTS);
	strcat(p_parts, ";");
#endif
	p_parts[strlen(p_parts) - 1] = '\0';
	cmdline_add(p_parts);
	cmdline_add(mtdparts_end);
	flash_switch_type(type);
	free(p_parts);
}

#ifdef CONFIG_CRYPT_PART_INFO
void mtdparts_cryptinfo_to_cmdline(const char *crypt_info)
{
#define CRYPT_CMDLINE_LENGTH 256
	char crypt_cmdline[CRYPT_CMDLINE_LENGTH] = {0};
	char part_name[FLASH_PARTITION_NAME_SIZE+1];
	struct partition_info *partition;
	int crypt_count = 0;
	const char *s = crypt_info;

	cmdline_add_noappend_space("cryptinfo=");
	for (; *s != '\0'; ) {
		char *p;

		memset(crypt_cmdline, 0x00, CRYPT_CMDLINE_LENGTH);

		p = strchr(s, '(');
		if (!p)
			break;

		if (p - s > FLASH_PARTITION_NAME_SIZE) {
			printf("partition name in CONFIG_ENCRYPT_PART_CFG is bigger than FLASH_PARTITION_NAME_SIZE %d\n", FLASH_PARTITION_NAME_SIZE);
			break;
		}
		memcpy(part_name, s, p - s);
		part_name[p - s] = '\0';
		s = p;

		partition = all_partition_get(part_name);
		if (partition == NULL) {
			printf("there is no partition name : %s", part_name);
			break;
		}
		if (crypt_count > 0)
			snprintf(crypt_cmdline, CRYPT_CMDLINE_LENGTH, ",0x%x@0x%x", partition->start_addr, partition->used_size);
		else
			snprintf(crypt_cmdline, CRYPT_CMDLINE_LENGTH, "0x%x@0x%x", partition->start_addr, partition->used_size);

		p = strchr(s, ')');
		if (!p)
			break;
		/* here +1 to include ')' */
		p += 1;
		if (p - s > CRYPT_CMDLINE_LENGTH - strlen(crypt_cmdline))
			break;
		memcpy(&crypt_cmdline[strlen(crypt_cmdline)], s, p - s);
		cmdline_add_noappend_space(crypt_cmdline);

		crypt_count++;

		s = p;
		if (*s != ',') { //last part
			break;
		}
		s++;
	}


	if (crypt_count > 0) {
		cmdline_add_noappend_space(" ");
		cmdline_add("cryptinfo_end");
	}
	free(crypt_cmdline);
}
#endif

#ifdef CONFIG_ENABLE_BOOT_FROM_USB

#define KERNEL_ADDR   KERNEL_START_ADDR
#define BUFF_SIZE     0x1000000
extern int usb_read(unsigned char *data, int datalen, char *filename);

void boot_from_usb_interface(char *filename)
{
	unsigned char *buff;
	int real_size;
	struct partition_info partition;

	if (!filename) {
		gxlogi("%s failed, filename is NULL", __func__);
		return;
	}

	buff = memalign(ARCH_DMA_MINALIGN, BUFF_SIZE);
	if (!buff) {
		gxlogi ("malloc failed\n");
		return ;
	}

	real_size = usb_read(buff, BUFF_SIZE, filename);
	if (real_size == -1) {
		gxlogi ("%s failed, may be %s file is not exist\n", __func__, filename);
		free(buff);
		return ;
	}
	gxlogi ("get file %s, size : %d\n", filename, real_size);
	set_image_buff(buff);

	memset(&partition, 0x00, sizeof(struct partition_info));
	partition.total_size = real_size;
	memcpy(partition.name, PARTITION_KERNEL, strlen(PARTITION_KERNEL));

	// Let's Go
	gxlogi ("Let's Start the kernel\n");
	doboot_kernel(&partition);

	free(buff);

}

static void boot_from_usb(void)
{
	boot_from_usb_interface(BOOT_FROM_USB_IMAGE_NAME);
}
#endif

#ifdef CONFIG_ENABLE_THIRDLIB
extern void boot_downloader(int argc, const char **argv);
#endif

#if defined(CONFIG_BOOT_KERNEL_FROM_FIXED_ADDR)
static void doboot_kernel_from_fixed_addr(void)
{
	struct partition_info kernel_part;

	memset(&kernel_part, 0, sizeof(struct partition_info));
	strcpy(kernel_part.name, "KERNEL");
	kernel_part.total_size = CONFIG_KERNEL_IMAGE_SIZE;
	kernel_part.partition_size = kernel_part.total_size;
	kernel_part.start_addr = CONFIG_KERNEL_IMAGE_ADDR;

	doboot_kernel(&kernel_part);
}
#endif

#ifdef CONFIG_EMBED_KERNEL_IMG
static void doboot_kernel_embed_kernel_img(void)
{
	extern unsigned long embed_kernel_img;
	extern unsigned long embed_kernel_img_end;
	struct partition_info kernel_part;

	memset(&kernel_part, 0, sizeof(struct partition_info));
	strcpy(kernel_part.name, "KERNEL");

	set_image_buff((void *)&embed_kernel_img);
	kernel_part.total_size = (unsigned long)&embed_kernel_img_end - (unsigned long)&embed_kernel_img;
	kernel_part.partition_size = kernel_part.total_size;

	printf("boot embed kernel img...\n");
	doboot_kernel(&kernel_part);
}
#endif

#ifdef CONFIG_LOAD_GX_RECOVERY_OS
static void doboot_kernel_gx_recovery_img(void)
{
	/* gx_recovery_img formart
	 *
	 *  ***********************
	 *  *  Recovery Loader    *
	 *  ***********************
	 *  *  Recovery Table     *
	 *  ***********************
	 *  *  Recovery OS        *
	 *  ***********************
	 *
	 * */
#define ROM_SEARCH_BACKUP_LOADER_SKIP_SIZE 0x10000 //64KB
#define RECOVER_TABLE_ALIGN_SIZE 0x1000 //4KB
#define RECOVER_LOADER_PART_ID 1
#define RECOVER_TABLE_PART_ID 1
#define RECOVER_OS_PART_ID 2
extern unsigned int backup_loader_addr;
	struct partition_info recovery_os_part;
	uint32_t flash_size = gxflash_get_info(GX_FLASH_CHIP_SIZE);
	uint32_t addr = backup_loader_addr;
	uint32_t magic_number;
	struct partition *recovery_partition;
	uint8_t *recovery_partition_buf = NULL;
	uint8_t *recovery_os_buf = NULL;
	uint32_t crc_value = 0;
	int found = 0;

	printf("Try booting gx_recovery_img...\n");

	recovery_partition = malloc(sizeof(*recovery_partition));
	if (recovery_partition == NULL) {
		printf("%s malloc recovery_partition failed\n", __func__);
		goto boot_recovery;
	}
	recovery_partition_buf = malloc(FLASH_PARTITION_MAX_SIZE);
	if (recovery_partition_buf == NULL) {
		printf("%s malloc recovery_partition_buf failed\n", __func__);
		goto boot_recovery;
	}

	/* If backup_loader_addr is zero, this indicates that the currently running
	 * code is the Loader that was factory-burned at address 0, not the Recovery Loader.
	 * We need find Recovery Loader first,
	 * */
	if (backup_loader_addr == 0) {
		printf("backup_loader_addr is 0, find Recovery Loader...\n");
		for (addr = ROM_SEARCH_BACKUP_LOADER_SKIP_SIZE; addr < flash_size; addr += ROM_SEARCH_BACKUP_LOADER_SKIP_SIZE) {
			gxflash_readdata(addr, (unsigned char *)&magic_number, 4);
			if (magic_number == 0x55aa55aa || magic_number == 0x55bb55bb)
				break;
		}

		if (addr >= flash_size) {
			printf("%s Can't find Recovery Loader\n", __func__);
			goto boot_recovery;
		}
		printf("Ok\n");
	}

	printf("Find Recovery Table...\n");
	/* Find Recovery Table */
	for (; addr < flash_size; addr += RECOVER_TABLE_ALIGN_SIZE) {
		if ((addr + FLASH_PARTITION_MAX_SIZE) >= flash_size)
			goto boot_recovery;
		gxflash_readdata(addr, recovery_partition_buf, FLASH_PARTITION_MAX_SIZE);
		if (mem_partition_init_v2(recovery_partition, recovery_partition_buf, FLASH_PARTITION_MAX_SIZE, 0, FLASH_PARTITION_SIZE) == 0)
			break;
	}
	if (addr >= flash_size) {
		printf("%s Can't find Recovery Table\n", __func__);
		goto boot_recovery;
	}
	printf("Ok\n");

	mem_partition_printf(recovery_partition);
	if (recovery_partition->count != 3) {
		printf("%s recovery_partition format error\n", __func__);
		goto boot_recovery;
	}

	memcpy(&recovery_os_part, &recovery_partition->tables[RECOVER_OS_PART_ID], sizeof(recovery_os_part));
	if (!recovery_os_part.crc32_enable) {
		printf("%s You must set the CRC of the recovery_os partition in flash.conf to true when creating the recovery image.\n", __func__);
		goto boot_recovery;
	}
	recovery_os_part.start_addr = addr + recovery_partition->tables[RECOVER_TABLE_PART_ID].total_size;

	recovery_os_buf = malloc(recovery_os_part.used_size);
	if (recovery_os_buf == NULL) {
		printf("%s malloc recovery_os_buf failed\n", __func__);
		goto boot_recovery;
	}
	partition_read(&recovery_os_part, 0, recovery_os_buf, recovery_os_part.used_size);
	crc_value = crc32(0, (unsigned char *)recovery_os_buf, recovery_os_part.used_size);
	if (crc_value != recovery_os_part.crc_verify) {
		printf("%s calc recovery_os part crc failed\n", __func__);
		goto boot_recovery;
	}

	found = 1;

boot_recovery:
	if (recovery_partition_buf != NULL)
		free(recovery_partition_buf);
	if (recovery_partition != NULL)
		free(recovery_partition);
	if (recovery_os_buf != NULL)
		free(recovery_os_buf);
	if (found == 1) {
		printf("Find gx recovery img at 0x%08x flash addr, and the size is %d\n", \
			recovery_os_part.start_addr, recovery_os_part.used_size);
		printf("boot gx recovery img...\n");
		doboot_kernel(&recovery_os_part);
	}

	printf("booting gx_recovery_img failed\n");

	return;
}
#endif

void simple_boot(int argc, const char **argv)
{
	struct partition_info *partition = NULL;

#ifdef CONFIG_ENABLE_TEE
#if defined(CONFIG_ENABLE_CMD) && CONFIG_BOOTDELAY > 0
	if (g_ree_kernel_addr != 0)
		run_kernel(g_ree_kernel_addr, g_ree_dtb_addr);
#endif
#endif

#ifdef CONFIG_ENABLE_BOOT_FROM_USB
	boot_from_usb();
#endif

#ifdef CONFIG_ENABLE_THIRDLIB
	boot_downloader(0, NULL);
#endif

#if defined(CONFIG_BOOT_KERNEL_FROM_FIXED_ADDR)
	doboot_kernel_from_fixed_addr();
#endif

#ifdef CONFIG_EMBED_KERNEL_IMG
	doboot_kernel_embed_kernel_img();
#endif

#ifdef CONFIG_LOAD_GX_RECOVERY_OS
	doboot_kernel_gx_recovery_img();
#endif

	if (argc > 1) {
		if (!strncasecmp(argv[1], PARTITION_OTA, FLASH_PARTITION_NAME_SIZE)) {
#ifdef CONFIG_ENABLE_OTA
			gxota_entry();
#endif
			gxlogi("warning: entry ota failed, entry kernel!\n");
			partition = all_partition_get(PARTITION_KERNEL);
		} else
			partition = all_partition_get(argv[1]);
	} else {
#ifdef CONFIG_ENABLE_OTA
		gxota_entry();
		gxlogi("warning: entry ota failed, entry kernel!\n");
#endif
		partition = all_partition_get(PARTITION_KERNEL);
	}

	if (partition == NULL)
		gxlogi("error: no partition found\n");
	else
		doboot_kernel(partition);
}
#endif /* #ifndef CONFIG_ECOS_OTA */
void run_kernel(u32 addr, unsigned long dtb_addr)
{
	char *param_to_kernel = NULL;

#ifdef CONFIG_ECOS_OTA
	boot_close_module();
	param_to_kernel = (char *)get_whole_cmdline() - 8;
#else
	rtc_printf();
	boot_close_module();
	param_to_kernel = (char *)bootstr_cmdline;
#endif

	if (addr == LOAD_KERNEL_ADDR) {
#ifdef CONFIG_ENABLE_TEE
#ifdef CONFIG_REE_LOADER
#if defined(CONFIG_ENABLE_CMD) && CONFIG_BOOTDELAY > 0
		if (g_ree_kernel_addr == 0) {
			g_ree_kernel_addr = addr;
			g_ree_dtb_addr = dtb_addr;
			gxteeloader_jump_teeos(0, 0);
		}
#else
		gxteeloader_jump_teeos(0, 0);
#endif
#else
#if defined(CONFIG_ENABLE_CMD) && CONFIG_BOOTDELAY > 0
		extern void return_from_tee(void);
		unsigned int kernel_addr = (unsigned int)return_from_tee;
		if (g_ree_kernel_addr == 0) {
			g_ree_kernel_addr = addr;
			g_ree_dtb_addr = dtb_addr;
			addr = g_tee_entry_addr;
		} else {
			kernel_addr = 0;
		}
#else
		unsigned int kernel_addr = addr;
		addr = g_tee_entry_addr;
#endif
#endif /*#ifdef CONFIG_REE_LOADER */
#endif /*#ifdef CONFIG_ENABLE_TEE */

#ifdef CONFIG_ENABLE_KERNEL_DTB
		if (dtb_addr)
			param_to_kernel = (char *)dtb_addr;
#endif

#ifdef CONFIG_ARCH_CKMMU
		void (*theKernel) ( unsigned int, char*) = (void (*) (unsigned int, char*))addr;
		(*theKernel)(CMDLINE_MAGIC, param_to_kernel);  /* MAGIC */
#elif defined(CONFIG_ARCH_ARM)
		void (*theKernel) ( unsigned int, unsigned int, char*) = (void (*) (unsigned int, unsigned int, char*))addr;
#if defined(CONFIG_ENABLE_TEE) && !defined(CONFIG_REE_LOADER)
		(*theKernel)(0, kernel_addr, param_to_kernel);
#else
#ifdef ENABLE_SECURE_ALIGN
		asm volatile("ldr     r6, =0xb002a114"); //pass secure align flag 0xb002a114 to kernel, only ecos kernel used this flag
#endif
		(*theKernel)(0, 0, param_to_kernel);
#endif
#else
		gxlogi("(%s %d)ERROR ARCH : %s\n", __func__, __LINE__, CONFIG_ARCH);
#endif
	} else {
#ifdef CONFIG_ARCH_CKMMU
		void (*theKernel) ( unsigned int, char*) = (void (*) (unsigned int, char*))addr;
		(*theKernel)(CMDLINE_MAGIC, param_to_kernel);  /* MAGIC */
#elif defined(CONFIG_ARCH_ARM)
		void (*theKernel) ( unsigned int, unsigned int, char*) = (void (*) (unsigned int, unsigned int, char*))addr;
		(*theKernel)(0, 0, param_to_kernel);
#else
		gxlogi("(%s %d)ERROR ARCH : %s\n", __func__, __LINE__, CONFIG_ARCH);
#endif
	}
}

#ifdef CONFIG_ENABLE_KERNEL_DTB
static int modify_dtb(void* kernel_image_buff, unsigned int kernel_image_len, unsigned int *kernel_offset)
{
	int ret = -1;
	char *dtb_addr = (char *)KERNEL_DTB_START_ADDR;
	unsigned int dtb_totalsize = KERNEL_DTB_SIZE;
#ifdef CONFIG_ECOS_OTA
	char *cmdline = (char *)get_whole_cmdline();
#else
	char *cmdline = (char *)((struct tag *)bootstr_cmdline)->u.cmdline.cmdline;
#endif

	if (kernel_image_len < dtb_totalsize)
		return ret;

	/* get dtb */
	memcpy(dtb_addr, kernel_image_buff, dtb_totalsize);

	if (fdt_check_header((void *)dtb_addr) < 0) {
		gxlogi("fdt header error. please check if Kernel Image contains DTB\n");
		return ret;
	}
	*kernel_offset = fdt_totalsize((void *)dtb_addr);
	if (fdt_open_into((void *)dtb_addr, (void *)dtb_addr, dtb_totalsize))
		return ret;
	if (fdt_chosen((void *)dtb_addr, cmdline))
		return ret;

	ret = 0;
	return ret;
}
#endif

#ifdef CONFIG_ENABLE_TEE
int get_tee(char *mem, unsigned int mem_len)
{
#define TEE_HEADER_LEN  0x1000  //uImageHeader + 4032
	struct kernel_loader *tee_loader = &uImage_load;
	int hdr_size;
	int img_size = 0;
	void *p = NULL;
	struct partition_info *partition = NULL;

	if (mem_len <= TEE_HEADER_LEN)
		return -1;

	partition = all_partition_get(PARTITION_TEE);
	if (partition == NULL)
		return -1;

	hdr_size = tee_loader->get_header_size();
	p = page_malloc(hdr_size);
	partition_read(partition, 0, p, hdr_size);
	if (tee_loader->check_magic((u32)p) == 0)
		img_size = tee_loader->get_image_size();

	page_free(p);
	p = NULL;

	if (!img_size) {
		g_tee_load_addr = (unsigned int)mem + TEE_HEADER_LEN;
		mem_len -= TEE_HEADER_LEN;
		img_size = partition->partition_size;
	} else
		g_tee_load_addr = (unsigned int)mem;

	g_tee_entry_addr = (unsigned int)mem + TEE_HEADER_LEN;

	if (img_size > mem_len) {
		gxlogi("error: TEE image size bigger than TEE mem size\n");
		return -1;
	}

	if (partition_read(partition, 0, (void*)g_tee_load_addr, img_size)) {
		gxlogi("error: read flash error!\n");
		return -1;
	}

	return img_size;
}
#endif

static int get_kernel_len(struct partition_info *partition)
{
	int kernel_len = 0;
	int kernel_offset = 0;
	int image_len = 0;

#ifdef CONFIG_ENABLE_KERNEL_DTB
	void *p = NULL;
	int fdt_header_size = sizeof(struct fdt_header);

	p = page_malloc(fdt_header_size);
	partition_read(partition, 0, p, fdt_header_size);

	if (fdt_check_header((void *)p) == 0)
		kernel_offset = fdt_totalsize((void *)p);

	page_free(p);
	p = NULL;
#endif

	image_len = get_image_len(partition, kernel_offset);

	if (image_len == -1 || image_len == 0)
		kernel_len = partition->partition_size;
	else
		kernel_len = image_len + kernel_offset;

	return kernel_len;
}

static struct signature_ops_t signature_ops = {0};

void register_signature(struct signature_ops_t *sign_ops)
{
	memcpy(&signature_ops, sign_ops, sizeof(struct signature_ops_t));
}

static struct crypt_ops_t crypt = {0};

void register_decrypt(struct crypt_ops_t *crypt_ops)
{
	memcpy(&crypt, crypt_ops, sizeof(struct crypt_ops_t));
}

#ifdef ENABLE_DVTCAS_VERIFY
int dvtcas_verify(struct partition_info *partition, unsigned char *signature, unsigned int sign_len, unsigned char *src, unsigned int src_len)
{
    unsigned char pub_key[256] = {0,};
    int ret = 0;

    if((src) && (src_len > 0) && (signature) && (sign_len = 256))
    {
        ret = rsa_pubkey_verif_fun(src, src_len, pub_key, signature);
    }

    return ret;
}

struct signature_ops_t dvtcas_signature=
{
    .verify_signature = dvtcas_verify,
};

void register_verify_for_dvtcas(void)
{
    register_signature(&dvtcas_signature);
}
#endif

#ifdef ENABLE_VERIFY
static int verify_other_partition(void)
{
	int num;
	int i;
	struct partition *flash_entry;
	void *img_buf = NULL;
	int img_len = 0;
	struct partition_info *partition;
	int extra_used_len = 0;

	num = flash_get_type();
	flash_entry = flash_entry_arr[num];

	/* no partition */
	if(flash_entry->count == 0)
		return -1;

	for (i = 0; i < flash_entry->count; i++) {
		partition = &flash_entry->tables[i];
		if (strcmp(partition->name, "BOOT") == 0
			|| strcmp(partition->name, PARTITION_TABLE) == 0
			|| strcmp(partition->name, PARTITION_KERNEL) == 0
			|| strcmp(partition->name, PARTITION_TEE) == 0
			|| strcmp(partition->name, PARTITION_OTA) == 0)
			continue;

		if (partition->crc32_enable == 0)
			continue;

		img_len = partition->partition_size;

#if defined(CONFIG_ARCH_ARM_SIRIUS)
		extra_used_len = EXTRA_VERIFY_USED_SIZE;
#else
		extra_used_len = 0;
#endif

		img_buf = page_malloc(img_len + extra_used_len);
		if (img_buf == NULL) {
			gxlogi("page malloc partition %d image buf failed : %s %s %d\n", partition->name, __FILE__, __func__, __LINE__);
			return -1;
		}

		partition_read(partition, 0, (char *)img_buf + extra_used_len, img_len);

		if (signature_ops.verify_signature) {
			unsigned int signature_len = 0;
			unsigned char *signature_buf = NULL; //A signaure buffer pointer that should point to buffer using malloc func to allocate
			if (signature_ops.get_signature != NULL)
				signature_ops.get_signature(partition, &signature_buf, &signature_len);
			if (signature_ops.verify_signature(partition, signature_buf, signature_len, (char *)img_buf + extra_used_len, img_len) != 0) {
				gxlogi("verify_%s signature failed: %s %s %d\n", partition->name,  __FILE__, __func__, __LINE__);
				return -1;
			}
			if (signature_buf != NULL) {
				free(signature_buf);
				signature_buf = NULL;
			}
		}

		if (img_buf != NULL) {
			page_free(img_buf);
			img_buf = NULL;
		}
	}

	return 0;
}
#endif

static int load_initrd(char *mem, unsigned int mem_len)
{
	struct partition_info *partition = NULL;

	partition = partition_get("rootfs");
	if (partition == NULL) {
		gxlogi("get partition rootfs failed: %s", __func__);
		return -1;
	}
	if (partition->partition_size > mem_len) {
		gxlogi("rootfs partition size > INIT_DRAM_SIZE: %s", __func__);
		return -1;
	}

	partition_read(partition, 0, mem, partition->partition_size);

	return 0;
}

static int check_ota_addr(char *ota_img_buf)
{
	unsigned int ota_addr = atoi(ota_img_buf + 19);
	if (!strncmp(ota_img_buf + 16, PARTITION_OTA, strlen(PARTITION_OTA))) {
		if (ota_addr != OTA_DRAM_START_ADDR) {
			gxlogi("error: OTA run addr %08x is not equal OTA load addr %08x\n", ota_addr, OTA_DRAM_START_ADDR);
			return -1;
		}
	}

	return 0;
}

int doboot_kernel(struct partition_info *partition)
{
	const char *kernel_names[] = {
#if defined(CONFIG_ENABLE_OTA) || defined(CONFIG_ENABLE_BBT_SLT_TEST)
		"ota.bin.gz",
		"ota.bin.lzma",
#endif
		"ecos.bin.lzma",
		"ecos.bin.gz",
		"vmlinux",
		"ecos.elf",
		NULL,
	};
	unsigned int kernel_offset = 0;
	unsigned long loadaddr_kernel = 0;
	unsigned long kernel_max_dramsize_start = 0;
	unsigned long kernel_max_dramsize_end = 0;
	unsigned long kernel_dramsize_start = 0;
	unsigned long kernel_dramsize_end = 0;
	unsigned long dtb_addr = 0;

#ifdef ENABLE_DVTCAS_VERIFY
    register_verify_for_dvtcas();
#endif

#ifdef CONFIG_ENABLE_OTA_FORCE_DRAM_ADDR
	if (!strcmp(partition->name, PARTITION_OTA)) {
		loadaddr_kernel = OTA_DRAM_START_ADDR;
		kernel_max_dramsize_start = OTA_DRAM_START_ADDR;
		kernel_max_dramsize_end = OTA_DRAM_END_ADDR;
		kernel_dramsize_start = OTA_DRAM_START_ADDR;
		kernel_dramsize_end = OTA_DRAM_END_ADDR;
	} else {
		loadaddr_kernel = LOAD_KERNEL_ADDR;
		kernel_max_dramsize_start = DRAMBASE;
		kernel_max_dramsize_end = LOADER_START_ADDR;
		kernel_dramsize_start = KERNEL_START_ADDR;
		kernel_dramsize_end = KERNEL_END_ADDR;
#ifdef ENABLE_VERIFY
		if (verify_other_partition() != 0)
			return -2;
#endif
	}
#else
	loadaddr_kernel = LOAD_KERNEL_ADDR;
	kernel_max_dramsize_start = DRAMBASE;
	kernel_max_dramsize_end = LOADER_START_ADDR;
	kernel_dramsize_start = KERNEL_START_ADDR;
	kernel_dramsize_end = KERNEL_END_ADDR;
#ifdef ENABLE_VERIFY
	if (verify_other_partition() != 0)
		return -2;
#endif
#endif

	if (partition != NULL) {
		int ret = 0;
		struct part_info part;
		int id = 0;
		void * kernel_image_buf = NULL;
		int kernel_image_len = 0;
		int tee_image_len = 0;
		int extra_used_len = 0;
#ifdef CONFIG_ENABLE_TEE
		void *tee_mem = NULL;
		unsigned int tee_mem_len;
#endif

		part.partition = partition;
		part.dest = loadaddr_kernel;

#if defined(CONFIG_ENABLE_OTA_FORCE_DRAM_ADDR) && defined(OTA_FORCE_MULTI_SECTION_FLASH)
		if (!strcmp(partition->name, PARTITION_OTA))
			partition->partition_size = gxota_get_bin_size();
		else
			partition->partition_size = partition->total_size - partition->reserved_size;
#else
		//for default partiton
		partition->partition_size = partition->total_size - partition->reserved_size;
#endif

		if (partition->partition_size > (kernel_max_dramsize_end - kernel_max_dramsize_start)) {
			gxlogi("error:partiton_size(0x%x) > max kernel dramsize(0x%x).\n",
					partition->partition_size, kernel_max_dramsize_end - kernel_max_dramsize_start);
			ret = -1;
			goto exit;
		}

		memset((void *)kernel_dramsize_start, 0, kernel_dramsize_end - kernel_dramsize_start);

#ifdef CONFIG_LOAD_IMAGE_LEN
		kernel_image_len = get_kernel_len(part.partition);
#else
		kernel_image_len = part.partition->partition_size;
#endif

#if defined(CONFIG_ARCH_ARM_SIRIUS)
		extra_used_len = EXTRA_VERIFY_USED_SIZE;
#else
		extra_used_len = 0;
#endif
		/* get kernel image */
		if (get_image_buff() == NULL) {
			kernel_image_buf = page_malloc(kernel_image_len + extra_used_len);
			if (kernel_image_buf == NULL) {
				gxlogi("page malloc kernel_image_buf failed : %s %s %d\n", __FILE__, __func__, __LINE__);
				ret = -1;
				goto exit;
			}
#if defined(CONFIG_ENABLE_OTA_FORCE_DRAM_ADDR) && defined(OTA_FORCE_MULTI_SECTION_FLASH)
			if (!strcmp(partition->name, PARTITION_OTA))
				gxota_load((char*)kernel_image_buf, kernel_image_len);
			else
				partition_read(part.partition, 0, (char*)kernel_image_buf + extra_used_len, kernel_image_len);
#else
			partition_read(part.partition, 0, (char*)kernel_image_buf + extra_used_len, kernel_image_len);
#endif
			set_image_buff((char*)kernel_image_buf + extra_used_len);
		}

#ifdef CONFIG_ENABLE_ROOTFS_INITRD
		load_initrd((char *)INITRD_DRAM_START_ADDR, INITRD_DRAM_SIZE);
#endif

#ifdef ENABLE_APP_DECRYPT
		if (crypt.app_dec) {
			if (crypt.app_dec(part.partition, get_image_buff(), kernel_image_len, get_image_buff(), kernel_image_len) != 0) {
				gxlogi("decrypt_%s  failed: %s %s %d\n", part.partition->name,  __FILE__, __func__, __LINE__);
				set_image_buff(NULL);
				ret = -2;
				goto exit;
			}
		}
#endif

#ifdef ENABLE_VERIFY
		/* kernel image signature verification
		 * kernel image start addr is get_image_buff()
		 * kernel image size is kernel_image_len
		 */
		if (signature_ops.verify_signature) {
			unsigned int signature_len = 0;
			unsigned char *signature_buf = NULL; //A signaure buffer pointer that should point to buffer using malloc func to allocate
			if (signature_ops.get_signature != NULL)
				signature_ops.get_signature(part.partition, &signature_buf, &signature_len);
			if (signature_ops.verify_signature(part.partition, signature_buf, signature_len, get_image_buff(), kernel_image_len) != 0) {
				gxlogi("verify_%s signature failed: %s %s %d\n", part.partition->name,  __FILE__, __func__, __LINE__);
				set_image_buff(NULL);
				ret = -2;
				goto exit;
			}
			if (signature_buf != NULL) {
				free(signature_buf);
				signature_buf = NULL;
			}
		}
#endif

		if (strcmp(partition->name, PARTITION_OTA) != 0) {
#ifdef CONFIG_ENABLE_KERNEL_DTB
			if (modify_dtb(get_image_buff(), kernel_image_len, &kernel_offset) < 0) {
				gxlogi("warning: The kernel images not have DTB image in header or fdt data is error %s %s %d\n", __FILE__, __func__, __LINE__);
				dtb_addr = 0;
			} else
				dtb_addr = KERNEL_DTB_START_ADDR;
#endif

#ifdef CONFIG_ENABLE_TEE
#ifdef CONFIG_REE_LOADER
			tee_mem = page_malloc(CMDLINE_TEEMEM_SIZE);
			if (tee_mem == NULL) {
				gxlogi("page malloc tee_mem failed : %s %s %d\n", __FILE__, __func__, __LINE__);
				ret = -1;
				goto exit;
			}

			tee_mem_len = CMDLINE_TEEMEM_SIZE;
#else
			tee_mem = (void *)(CMDLINE_TEEMEM_START + GX_DRAM_VIRTUAL_OFFSET);
			tee_mem_len = CMDLINE_TEEMEM_SIZE;
#endif
			tee_image_len = get_tee((char *)tee_mem, tee_mem_len);
			if (tee_image_len < 0) {
				gxlogi("error: %s %s %d\n", __FILE__, __func__, __LINE__);
				ret = -1;
				goto exit;
			}

#ifdef CONFIG_REE_LOADER
			if (gxteeloader_load_teeos(tee_mem, tee_mem_len) < 0) {
				gxlogi("error: %s %s %d\n", __FILE__, __func__, __LINE__);
				ret = -1;
				goto exit;
			}
#else
#ifdef ENABLE_APP_DECRYPT
			if (crypt_ops && crypt_ops->app_dec) {
				struct partition_info *tee_partition = NULL;

				tee_partition = all_partition_get(PARTITION_TEE);
				if (crypt_ops->app_dec(tee_partition, (unsigned char*)(g_tee_load_addr), tee_image_len, (unsigned char*)(g_tee_load_addr), tee_image_len) != 0) {
					gxlogi("decrypt_%s  failed: %s %s %d\n", part.partition->name,  __FILE__, __func__, __LINE__);
					set_image_buff(NULL);
					ret = -2;
					goto exit;
				}
			}
#endif
#ifdef ENABLE_VERIFY
			/* TEE image signature verification
			 * TEE image start addr is (CMDLINE_TEEMEM_START + GX_DRAM_VIRTUAL_OFFSET)
			 * TEE image len is tee_image_len
			 */
			if (signature_ops.verify_signature) {
				unsigned int signature_len = 0;
				unsigned char *signature_buf = NULL; //A signaure buffer pointer that should point to buffer using malloc func to allocate
				struct partition_info *tee_partition = NULL;

				tee_partition = all_partition_get(PARTITION_TEE);
				if (signature_ops.get_signature != NULL)
					signature_ops.get_signature(tee_partition, &signature_buf, &signature_len);
				if (signature_ops.verify_signature(tee_partition, signature_buf, signature_len,(unsigned char*)(g_tee_load_addr), tee_image_len) != 0) {
					gxlogi("verify_%s signature failed: %s %s %d\n", PARTITION_TEE,  __FILE__, __func__, __LINE__);
					ret = -2;
					goto exit;
				}
				if (signature_buf != NULL) {
					free(signature_buf);
					signature_buf = NULL;
				}
			}
#endif
#endif
#endif
		}

#ifdef CONFIG_ENABLE_OTA
		if (!strncmp(part.partition->name, PARTITION_OTA, strlen(PARTITION_OTA))) {
			if (check_ota_addr(get_image_buff()) != 0) {
				set_image_buff(NULL);
				ret = -1;
				goto exit;
			}
		}
#endif

		while (kernel_names[id]) {
			if (load_from_memory(&part, kernel_names[id], get_image_buff(), kernel_offset) == 0) {
				run_kernel(part.dest, dtb_addr);
				ret = 0;
				goto exit;
			}
			id++;
		}
#ifdef CONFIG_ENABLE_LIB
		ret = -1;
		goto exit;
#endif
		gxlogi("not find image/fs, read partition directly.\n");
		memcpy((void*)part.dest, (void *)((char *)get_image_buff()+kernel_offset), part.partition->partition_size);
		run_kernel(part.dest, dtb_addr);
exit:
		if (kernel_image_buf != NULL)
			page_free(kernel_image_buf);
#ifdef CONFIG_ENABLE_TEE
#ifdef CONFIG_REE_LOADER
		if (tee_mem != NULL)
			page_free(tee_mem);
#endif
#endif
		return ret;
	}
	gxlogi("No found kernel partition!!!\n");

	return -1;
}

#ifdef CONFIG_ENABLE_LOGO
#define LOGO_MAX_SIZE		0x200000
extern char logo_partition_name[];
extern char logo_file_name[];
extern enum cvbs_mode_enum g_cvbs_mode;
extern enum ypbpr_hdmi_mode_enum g_ypbpr_hdmi_mode;
void gx_show_logo(void)
{
	u32 logo_max_size       = LOGO_MAX_SIZE;
	struct part_info part;
	struct partition_info *logo_partition;

  aout_i2s_play_zero();

	logo_partition = all_partition_get(logo_partition_name);
	if (logo_partition != NULL) {
		part.partition = logo_partition;
		if((logo_max_size < part.partition->partition_size) && (strcmp(logo_partition_name, PARTITION_LOGO) == 0)) {
			gxlogi("error:logo_max_size(0x%x) < partition_size(0x%x).\n", logo_max_size, part.partition->partition_size);
			return;
		}
		part.dest = (u32)page_malloc(logo_max_size);
		partition_read(part.partition, 0, (void*)part.dest, part.partition->partition_size);
		platform_show_logo((u8*)part.dest, logo_max_size);
		page_free((u8*)part.dest);
	}
}
#endif

