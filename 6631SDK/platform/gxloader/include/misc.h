#ifndef __MISC_H__
#define __MISC_H__

#define FLASH_TYPE_SPI_NOR	(0x1)
#define FLASH_TYPE_SPI_NAND	(0x2)
#define FLASH_TYPE_NAND		(0x3)
#define FLASH_TYPE_NULL		(0xff)

#define FLASH_MAGIC_SPINOR_NAND		(0x55aa55aa)
#define FLASH_MAGIC_SPINAND		(0x55bb55bb)

struct adaptive_flash {
	char flash_type;
	int (*read_flash)(unsigned int addr, unsigned int size, unsigned char *buffer);
};

extern struct adaptive_flash adaptive;

#endif
