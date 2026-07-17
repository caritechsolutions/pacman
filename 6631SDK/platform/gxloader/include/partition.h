#ifndef __PARTITION_H__
#define __PARTITION_H__

//#define CONFIG_PARTITION_V5

#define MAGIC_SIZE                      (4)
#define PARTITION_NUM_SIZE              (1)

#define PARTITION_MAGIC                  0XAABCDEFA
#define FLASH_PARTITION_SIZE            (512)
#define FLASH_PARTITION_NUM             (12)
#define FLASH_PARTITION_HEAD_SIZE       (24)
#define FLASH_PARTITION_V1_NAME_SIZE    (7)
#define FLASH_PARTITION_CRC_ADDR     (FLASH_PARTITION_HEAD_SIZE * 17 + MAGIC_SIZE + PARTITION_NUM_SIZE)
#define FLASH_PARTITION_VERSION_ADDR (FLASH_PARTITION_HEAD_SIZE * FLASH_PARTITION_NUM + MAGIC_SIZE + PARTITION_NUM_SIZE)

#define PARTITION_V2_MAGIC                  0XDD1C2BFA
#define FLASH_PARTITION_V2_SIZE            (1024)
#define FLASH_PARTITION_V2_NUM             (24)
#define FLASH_PARTITION_V2_HEAD_SIZE       (28)
#define FLASH_PARTITION_V2_NAME_SIZE       (7)
#define FLASH_PARTITION_V2_CRC_ADDR   \
                    (FLASH_PARTITION_V2_HEAD_SIZE * (FLASH_PARTITION_V2_NUM + 5) + MAGIC_SIZE + PARTITION_NUM_SIZE)
#define FLASH_PARTITION_V2_VERSION_ADDR   \
                    (FLASH_PARTITION_V2_HEAD_SIZE * FLASH_PARTITION_V2_NUM + MAGIC_SIZE + PARTITION_NUM_SIZE)

#define PARTITION_V3_MAGIC                  0XEE2D3C0B

#define PARTITION_V3_VERSION1                     0x3
#define PARTITION_V3_VERSION_OFS                   4
#define PARTITION_V3_SIZE_OFS                      5
#define PARTITION_V3_MAGIC_OFS                     0

#define PARTITION_V4_MAGIC                  0XFF3E4D1C
#define FLASH_PARTITION_V4_SIZE            (3072)
#define FLASH_PARTITION_V4_NUM             (84)
#define FLASH_PARTITION_V4_HEAD_SIZE       (28)
#define FLASH_PARTITION_V4_NAME_SIZE       (7)
#define FLASH_PARTITION_V4_CRC_ADDR   \
                    (MAGIC_SIZE + PARTITION_NUM_SIZE + FLASH_PARTITION_V4_HEAD_SIZE * FLASH_PARTITION_V4_NUM + sizeof(uint16_t) * FLASH_PARTITION_V4_NUM)
#define FLASH_PARTITION_V4_VERSION_ADDR   \
	            (MAGIC_SIZE + PARTITION_NUM_SIZE + FLASH_PARTITION_V4_HEAD_SIZE * FLASH_PARTITION_V4_NUM)

#define PARTITION_V5_MAGIC                  0XCCABEF12
#define FLASH_PARTITION_V5_SIZE            (8192)
#define FLASH_PARTITION_V5_NUM             (84)
#define FLASH_PARTITION_V5_HEAD_SIZE       (84)
#define FLASH_PARTITION_V5_NAME_SIZE       (63)
#define FLASH_PARTITION_V5_CRC_ADDR   \
		                (MAGIC_SIZE + PARTITION_NUM_SIZE + FLASH_PARTITION_V5_HEAD_SIZE * FLASH_PARTITION_V5_NUM + sizeof(uint16_t) * FLASH_PARTITION_V5_NUM)
#define FLASH_PARTITION_V5_VERSION_ADDR   \
		                (MAGIC_SIZE + PARTITION_NUM_SIZE + FLASH_PARTITION_V5_HEAD_SIZE * FLASH_PARTITION_V5_NUM)

#define PARTITION_BOOT               ("BOOT")
#define PARTITION_V_SET              ("V_SET")
#define PARTITION_LOGO               ("LOGO")
#define PARTITION_OTA                ("OTA")
#define PARTITION_I_OEM              ("I_OEM")
#define PARTITION_V_OEM              ("V_OEM")
#define PARTITION_KERNEL             ("KERNEL")
#define PARTITION_ROOT               ("ROOT")
#define PARTITION_TABLE              ("TABLE")
#define PARTITION_SIGNAT             ("SIGNAT")
#define PARTITION_TEE                ("TEE")

#ifdef CONFIG_PARTITION_V5
#define FLASH_PARTITION_MAX_NUM		FLASH_PARTITION_V5_NUM
#define FLASH_PARTITION_MAX_SIZE	FLASH_PARTITION_V5_SIZE
#define FLASH_PARTITION_NAME_SIZE       FLASH_PARTITION_V5_NAME_SIZE
#else
#define FLASH_PARTITION_MAX_NUM		FLASH_PARTITION_V4_NUM
#define FLASH_PARTITION_MAX_SIZE	FLASH_PARTITION_V4_SIZE
#define FLASH_PARTITION_NAME_SIZE       FLASH_PARTITION_V4_NAME_SIZE
#endif

#define RAW     0
#define ROMFS   1
#define CRAMFS  2
#define JFFS2   3
#define YAFFS2  4
#define RAMFS   5
#define SQUASHFS 6
#define UBIFS   7
#define HIDE    126
#define MINIFS  127

#define PARTITION_VERSION 102

/*
 * VERION 1.0.0 (<= V1.0.1-5)
 * +----------------------------+
 * |            512             |
 * +-------+-------+------------+
 * |   4   |   1   |    ...     |
 * +-------+-------+------------+
 * |       |       |            |
 * | MAGIC | COUNT | BASE TABLE |
 * |       |       |            |
 * +-------+-------+------------+
 *
 * VERION 1.0.2 ( <= V1.0.2-0-RC5)
 * +--------------------------------------------~----------------------------------------------+
 * |                                           512                                             |
 * +-------+-------+------------------------+---~----+---------+--------+----------+-----------+
 * |       |       |           360          |        |         |        |          |           |
 * |   4   |   1   +------------+-----------+  141   |    1    |    1   |    1     |     4     |
 * |       |       |  17 * 24   |  17 * 4   |        |         |        |          |           |
 * +-------+-------+------------+-----------+---~----+---------+--------+----------+-----------+
 * |       |       |            |           |        |         |        |          |           |
 * | MAGIC | COUNT | BASE TABLE | CRC TABLE | UNUSED | PROTECT | CRC_EN | PART VER | TABLE CRC |
 * |       |       |            |           |        |         |        |          |           |
 * +-------+-------+------------+-----------+---~----+---------+--------+----------+-----------+
 * |0      |4      |5           |413                 |505      |506     |507        |508
 *
 * VERION 1.0.3
 * +---------------------------------------------------------------~----------------------------------------------+
 * |                                                       512                                                    |
 * +-------+-------+-------------------------------------------+---~----+---------+--------+----------+-----------+
 * |       |       |                                           |        |         |        |          |           |
 * |   4   |   1   +------------+-----------+------+-----------+   45   |    1    |    1   |    1     |     4     |
 * |       |       |  12 * 24   |  12 * 2   |  96  |  12 * 4   |        |         |        |          |           |
 * +-------+-------+------------+-----------+------+-----------+---~----+---------+--------+----------+-----------+
 * |       |       |            |           |      |           |        |         |        |          |           |
 * | MAGIC | COUNT | BASE TABLE | VER TABLE |  UN  | CRC TABLE | UNUSED | PROTECT | CRC_EN | PART VER | TABLE CRC |
 * |       |       |            |           |      |           |        |         |        |          |           |
 * +-------+-------+------------+-----------+------+-----------+---~----+---------+--------+----------+-----------+
 * |0      |4      |5           |293               |413                 |505      |506     |507        |508
 */

/*
 * VERION 1.0.4
 * +----------------------------~------------------------------------------------------------------------+
 * |                                                 512                                                 |
 * +-------+-------+------------+--------------------+---------+---------+--------+----------+-----------+
 * |       |       |            |      |             |         |         |        |          |           |
 * |   4   |   1   |  24 * 12   |  120 |    4 * 12   |   44    |    1    |    1   |    1     |     4     |
 * |       |       |            |      |             |         |         |        |          |           |
 * +-------+-------+------------+--------------------+---------+---------+--------+----------+-----------+
 * |       |       |            |      |             |         |         |        |          |           |
 * | MAGIC | COUNT |   TABLES   |  UN  |  CRC TABLE  |   UN    | PROTECT | CRC_EN | PART VER | TABLE CRC |
 * |       |       |            |      |             |         |         |        |          |           |
 * +-------+-------+------------+--------------------+---------+---------+--------+----------+-----------+
 * |0      |4      |5           |293   |413          |461      |505      |506     |507       |508
 *
 * V2
 * +----------------------------~--------------------------------------------------------------------------+
 * |                                             1024                                                      |
 * +-------+-------+------------+-----------+-----------+--------+---------+--------+----------+-----------+
 * |       |       |            |           |           |        |         |        |          |           |
 * |   4   |   1   |  28 * 24   |    140    |   4 * 24  |   104  |    1    |    1   |    1     |     4     |
 * |       |       |            |           |           |        |         |        |          |           |
 * +-------+-------+------------+-----------+-----------+--------+---------+--------+----------+-----------+
 * |       |       |            |           |           |        |         |        |          |           |
 * | MAGIC | COUNT |   TABLES   |  VERSION  | CRC TABLE |   UN   | PROTECT | CRC_EN | PART VER | TABLE CRC |
 * |       |       |            |   TABLE   |           |        |         |        |          |           |
 * +-------+-------+------------+-----------+-----------+--------+---------+--------|----------|-----------+
 * |0      |4      |5           |677        |817        |913     |1017     |1018    |1019      |1020
 *
 * V4
 * +-------------------------------------------------------------------------------------------------------+
 * |                                                3072                                                   |
 * +-------+-------+------------+-----------+-----------+--------+---------+--------+----------+-----------+
 * |       |       |            |           |           |        |         |        |          |           |
 * |   4   |   1   |  28 * 84   |  2 * 84   |  4 * 84   |   204  |    1    |    1   |    1     |     4     |
 * |       |       |            |           |           |        |         |        |          |           |
 * +-------+-------+------------+-----------+-----------+--------+---------+--------+----------+-----------+
 * |       |       |            |           |           |        |         |        |          |           |
 * | MAGIC | COUNT |   TABLES   |  VERSION  | CRC TABLE |   UN   | PROTECT | CRC_EN | PART VER | TABLE CRC |
 * |       |       |            |  TABLE    |           |        |         |        |          |           |
 * +-------+-------+------------+-----------+-----------+--------+---------+--------|----------|-----------+
 * |0      |4      |5           |2357       |2525       |2861    |3065     |3066    |3067      |3068       |
 *
 * V5
 * +-------------------------------------------------------------------------------------------------------+
 * |                                                8192                                                   |
 * +-------+-------+------------+-----------+-----------+--------+---------+--------+----------+-----------+
 * |       |       |            |           |           |        |         |        |          |           |
 * |   4   |   1   |  84 * 84   |  2 * 84   |  4 * 84   |   620  |    1    |    1   |    1     |     4     |
 * |       |       |            |           |           |        |         |        |          |           |
 * +-------+-------+------------+-----------+-----------+--------+---------+--------+----------+-----------+
 * |       |       |            |           |           |        |         |        |          |           |
 * | MAGIC | COUNT |   TABLES   |  VERSION  | CRC TABLE |   UN   | PROTECT | CRC_EN | PART VER | TABLE CRC |
 * |       |       |            |  TABLE    |           |        |         |        |          |           |
 * +-------+-------+------------+-----------+-----------+--------+---------+--------|----------|-----------+
 * |0      |4      |5           |7061       |7229       |7565    |8185     |8186    |8187      |8188       |
 */

struct partition_info {
	char     name[FLASH_PARTITION_NAME_SIZE + 1];
	uint32_t total_size;
	uint32_t used_size;
	uint32_t reserved_size;
	uint32_t start_addr;
	char     file_system_type;
	char     mode;
	uint8_t  id;
	uint8_t  update_tags;

	uint32_t partition_size;
	uint32_t crc_verify;
	uint32_t crc32;
	uint16_t version;
	char     crc32_enable;
	char     write_protect;
	uint8_t  mtd_id;
} __attribute__ ((packed));

struct partition {
	unsigned int magic;                                      // 0-3
	unsigned char count;                                     // 4
	struct partition_info tables[FLASH_PARTITION_MAX_NUM];    // 5-364


	unsigned char write_protect;                             // 505
	unsigned char crc32_enable;                              // 506
	char version;                                            // 507
	unsigned int crc_verify;                                 // 508-512
	unsigned int table_version;                              // table version
	struct partition_io_ops *ops;                            //为了兼容
	int fd;                                                  //为了兼容
} __attribute__ ((packed));

#define COMPRESS_MAGIC "zlibmode"

struct compress_part_head {
	char name[FLASH_PARTITION_NAME_SIZE + 1];
	uint32_t compress_size;
} __attribute__ ((packed));

struct compress_part_info {
	struct compress_part_head head;
	unsigned char *buf;
};

struct compress_bin_info {
	struct partition table;
	unsigned int part_ctr;
	struct compress_part_info part[FLASH_PARTITION_MAX_NUM];
};

struct partition_table_info {
	unsigned int start;
	unsigned int end;
	unsigned int skip;
};

#define K (1024)
#define M (1024*1024)
#define AUTO 0xEFFFFFFF

int partition_init(struct partition_table_info *table_info);
struct partition_info *partition_get(const char* name);
int  partition_check (struct partition_info *partition, void *buf, uint32_t size);// 1:ok, 0: error
int  partition_read  (struct partition_info *partition, uint32_t offset, void *buf, uint32_t size);
int  partition_write (struct partition_info *partition, uint32_t offset, void *buf, uint32_t size);
int  partition_erase (struct partition_info *partition);
int  partition_update (struct partition_info *partition, void *buf, uint32_t size);
int  partition_save  (void);
void partition_printf(void);

int mem_partition_init(char *addr, struct partition *mem_partition_entry);
int mem_partition_init_v2(struct partition *mem_partition_entry, void *mem_buf, unsigned int len, unsigned int search_offset, unsigned int search_skip);
void mem_partition_printf(struct partition *mem_partition_entry);

char *get_fstype(int type);
void all_partition_printf(void);
int all_partition_init(struct partition_table_info *info);
struct partition* all_partition_update(void);
struct partition_info *all_partition_get(const char* name);
struct partition_info *all_partition_get_by_id(unsigned int id);

void partition_lock(void);
void partition_unlock(void);

extern unsigned char embed_flash_table[];

#endif
