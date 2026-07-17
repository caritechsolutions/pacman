#include <fat.h>
#include <usb.h>
#include <stdio.h>
#include <string.h>

/*************** part_dos.h *******************/
#ifndef _DISK_PART_DOS_H
#define _DISK_PART_DOS_H

#define DOS_PART_DISKSIG_OFFSET	0x1b8
#define DOS_PART_TBL_OFFSET	0x1be
#define DOS_PART_MAGIC_OFFSET	0x1fe
#define DOS_PBR_FSTYPE_OFFSET	0x36
#define DOS_PBR32_FSTYPE_OFFSET	0x52
#define DOS_PBR_MEDIA_TYPE_OFFSET	0x15
#define DOS_MBR	0
#define DOS_PBR	1

typedef struct dos_partition {
	unsigned char boot_ind;		/* 0x80 - active			*/
	unsigned char head;		/* starting head			*/
	unsigned char sector;		/* starting sector			*/
	unsigned char cyl;		/* starting cylinder			*/
	unsigned char sys_ind;		/* What partition type			*/
	unsigned char end_head;		/* end head				*/
	unsigned char end_sector;	/* end sector				*/
	unsigned char end_cyl;		/* end cylinder				*/
	unsigned char start4[4];	/* starting sector counting from 0	*/
	unsigned char size4[4];		/* nr of sectors in partition		*/
} dos_partition_t;

#endif	/* _DISK_PART_DOS_H */
/********************************************/

#define HAVE_BLOCK_DEVICE
#ifdef HAVE_BLOCK_DEVICE

/* Convert char[4] in little endian format to the host format integer
 */
static inline int le32_to_int(unsigned char *le32)
{
    return ((le32[3] << 24) +
	    (le32[2] << 16) +
	    (le32[1] << 8) +
	     le32[0]
	   );
}

static inline int is_extended(int part_type)
{
    return (part_type == 0x5 ||
	    part_type == 0xf ||
	    part_type == 0x85);
}

static inline int is_bootable(dos_partition_t *p)
{
	return p->boot_ind == 0x80;
}

static void print_one_part(dos_partition_t *p, int ext_part_sector,
			   int part_num, unsigned int disksig)
{
	int lba_start = ext_part_sector + le32_to_int (p->start4);
	int lba_size  = le32_to_int (p->size4);

	printf("%3d\t%-10d\t%-10d\t%08x-%02x\t%02x%s%s\n",
		part_num, lba_start, lba_size, disksig, part_num, p->sys_ind,
		(is_extended(p->sys_ind) ? " Extd" : ""),
		(is_bootable(p) ? " Boot" : ""));
}

static int test_block_type(unsigned char *buffer)
{
	if((buffer[DOS_PART_MAGIC_OFFSET + 0] != 0x55) ||
	    (buffer[DOS_PART_MAGIC_OFFSET + 1] != 0xaa) ) {
		return (-1);
	} /* no DOS Signature at all */
	if (strncmp((char *)&buffer[DOS_PBR_FSTYPE_OFFSET],"FAT",3)==0 ||
	    strncmp((char *)&buffer[DOS_PBR32_FSTYPE_OFFSET],"FAT32",5)==0) {
		return DOS_PBR; /* is PBR */
	}
	return DOS_MBR;	    /* Is MBR */
}


int test_part_dos (block_dev_desc_t *dev_desc)
{
	ALLOC_CACHE_ALIGN_BUFFER(unsigned char, buffer, dev_desc->blksz);

	if (dev_desc->block_read(dev_desc->dev, 0, 1, (ulong *) buffer) != 1)
		return -1;

	if (test_block_type(buffer) != DOS_MBR)
		return -1;

	return 0;
}

/*  Print a partition that is relative to its Extended partition table
 */
static void print_partition_extended(block_dev_desc_t *dev_desc,
				     int ext_part_sector, int relative,
				     int part_num, unsigned int disksig)
{
	ALLOC_CACHE_ALIGN_BUFFER(unsigned char, buffer, dev_desc->blksz);
	dos_partition_t *pt;
	int i;

	if (dev_desc->block_read(dev_desc->dev, ext_part_sector, 1, (ulong *) buffer) != 1) {
		printf ("** Can't read partition table on %d:%d **\n",
			dev_desc->dev, ext_part_sector);
		return;
	}
	i=test_block_type(buffer);
	if (i != DOS_MBR) {
		printf ("bad MBR sector signature 0x%02x%02x\n",
			buffer[DOS_PART_MAGIC_OFFSET],
			buffer[DOS_PART_MAGIC_OFFSET + 1]);
		return;
	}

	if (!ext_part_sector)
		disksig = le32_to_int(&buffer[DOS_PART_DISKSIG_OFFSET]);

	/* Print all primary/logical partitions */
	pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
	for (i = 0; i < 4; i++, pt++) {
		/*
		 * fdisk does not show the extended partitions that
		 * are not in the MBR
		 */

		if ((pt->sys_ind != 0) &&
		    (ext_part_sector == 0 || !is_extended (pt->sys_ind)) ) {
			print_one_part(pt, ext_part_sector, part_num, disksig);
		}

		/* Reverse engr the fdisk part# assignment rule! */
		if ((ext_part_sector == 0) ||
		    (pt->sys_ind != 0 && !is_extended (pt->sys_ind)) ) {
			part_num++;
		}
	}

	/* Follows the extended partitions */
	pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
	for (i = 0; i < 4; i++, pt++) {
		if (is_extended (pt->sys_ind)) {
			int lba_start = le32_to_int (pt->start4) + relative;

			print_partition_extended(dev_desc, lba_start,
				ext_part_sector == 0  ? lba_start : relative,
				part_num, disksig);
		}
	}

	return;
}


/*  Print a partition that is relative to its Extended partition table
 */
static int get_partition_info_extended (block_dev_desc_t *dev_desc, int ext_part_sector,
				 int relative, int part_num,
				 int which_part, disk_partition_t *info,
				 unsigned int disksig)
{
	ALLOC_CACHE_ALIGN_BUFFER(unsigned char, buffer, dev_desc->blksz);
	dos_partition_t *pt;
	int i;

	if (dev_desc->block_read (dev_desc->dev, ext_part_sector, 1, (ulong *) buffer) != 1) {
		printf ("** Can't read partition table on %d:%d **\n",
			dev_desc->dev, ext_part_sector);
		return -1;
	}
	if (buffer[DOS_PART_MAGIC_OFFSET] != 0x55 ||
		buffer[DOS_PART_MAGIC_OFFSET + 1] != 0xaa) {
		printf ("bad MBR sector signature 0x%02x%02x\n",
			buffer[DOS_PART_MAGIC_OFFSET],
			buffer[DOS_PART_MAGIC_OFFSET + 1]);
		return -1;
	}

#ifdef CONFIG_PARTITION_UUIDS
	if (!ext_part_sector)
		disksig = le32_to_int(&buffer[DOS_PART_DISKSIG_OFFSET]);
#endif

	/* Print all primary/logical partitions */
	pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
	for (i = 0; i < 4; i++, pt++) {
		/*
		 * fdisk does not show the extended partitions that
		 * are not in the MBR
		 */
		if (((pt->boot_ind & ~0x80) == 0) &&
		    (pt->sys_ind != 0) &&
		    (part_num == which_part) &&
		    (is_extended(pt->sys_ind) == 0)) {
			info->blksz = 512;
			info->start = ext_part_sector + le32_to_int (pt->start4);
			info->size  = le32_to_int (pt->size4);
			switch(dev_desc->if_type) {
				case IF_TYPE_IDE:
				case IF_TYPE_SATA:
				case IF_TYPE_ATAPI:
					sprintf ((char *)info->name, "hd%c%d",
						'a' + dev_desc->dev, part_num);
					break;
				case IF_TYPE_SCSI:
					sprintf ((char *)info->name, "sd%c%d",
						'a' + dev_desc->dev, part_num);
					break;
				case IF_TYPE_USB:
					sprintf ((char *)info->name, "usbd%c%d",
						'a' + dev_desc->dev, part_num);
					break;
				case IF_TYPE_DOC:
					sprintf ((char *)info->name, "docd%c%d",
						'a' + dev_desc->dev, part_num);
					break;
				default:
					sprintf ((char *)info->name, "xx%c%d",
						'a' + dev_desc->dev, part_num);
					break;
			}
			/* sprintf(info->type, "%d, pt->sys_ind); */
			sprintf ((char *)info->type, "U-Boot");
			info->bootable = is_bootable(pt);
#ifdef CONFIG_PARTITION_UUIDS
			sprintf(info->uuid, "%08x-%02x", disksig, part_num);
#endif
			return 0;
		}

		/* Reverse engr the fdisk part# assignment rule! */
		if ((ext_part_sector == 0) ||
		    (pt->sys_ind != 0 && !is_extended (pt->sys_ind)) ) {
			part_num++;
		}
	}

	/* Follows the extended partitions */
	pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
	for (i = 0; i < 4; i++, pt++) {
		if (is_extended (pt->sys_ind)) {
			int lba_start = le32_to_int (pt->start4) + relative;

			return get_partition_info_extended (dev_desc, lba_start,
				 ext_part_sector == 0 ? lba_start : relative,
				 part_num, which_part, info, disksig);
		}
	}
	return -1;
}

int get_partitions_extended(block_dev_desc_t *dev_desc,
				 int ext_part_sector, int relative)
{
	ALLOC_CACHE_ALIGN_BUFFER(unsigned char, buffer, dev_desc->blksz);
	dos_partition_t *pt;
	int part_num = 0;
	int i;

	if (dev_desc->block_read(dev_desc->dev, ext_part_sector, 1, (ulong *)buffer) != 1) {
		printf("** Can't read partition table on %d:%d **\n",
			dev_desc->dev, ext_part_sector);
		return 0;
	}

	i = test_block_type(buffer);
	if (i != DOS_MBR) {
		printf("bad MBR sector signature 0x%02x%02x\n",
			buffer[DOS_PART_MAGIC_OFFSET],
			buffer[DOS_PART_MAGIC_OFFSET + 1]);
		return 0;
	}

	pt = (dos_partition_t *)(buffer + DOS_PART_TBL_OFFSET);
	for (i = 0; i < 4; i++, pt++) {
		if (pt->sys_ind && !is_extended(pt->sys_ind))
			part_num++;
		else if (pt->sys_ind) {
			part_num += ext_part_sector == 0 ? 1 : 0;
			int lba_start = le32_to_int(pt->start4) + relative;
			part_num += get_partitions_extended(dev_desc, lba_start,
						ext_part_sector == 0 ? lba_start : relative);
		}
	}
	return part_num;
}

static int get_dos_partitions_index(block_dev_desc_t *dev_desc, const char *volume,
				int part_num, int ext_part_sector, int relative)
{
	ALLOC_CACHE_ALIGN_BUFFER(unsigned char, buffer, dev_desc->blksz);
	dos_partition_t *pt;
	boot_sector *bs;
	volume_info *volinfo;
	disk_partition_t info;
	uint8_t *block;
	int i;

	if (dev_desc->block_read(dev_desc->dev, ext_part_sector, 1, (ulong *) buffer) != 1) {
		printf ("** Can't read partition table on %d:%d **\n",
			dev_desc->dev, ext_part_sector);
		return -1;
	}

	i = test_block_type(buffer);
	if (i != DOS_MBR) {
		printf ("bad MBR sector signature 0x%02x%02x\n",
			buffer[DOS_PART_MAGIC_OFFSET],
			buffer[DOS_PART_MAGIC_OFFSET + 1]);
		return -1;
	}

	pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);

	for (i = 0; i < 4; i++, pt++) {
		if (pt->sys_ind && !is_extended(pt->sys_ind)) {
			info.blksz = 512;
			info.start = ext_part_sector + le32_to_int (pt->start4);
			block = memalign(ARCH_DMA_MINALIGN, info.blksz);
			if (dev_desc->block_read(dev_desc->dev, info.start, 1, block) != 1) {
				printf("**Read Partition %d DBR Error **\n", part_num);
				return -1;
			}

			bs = (boot_sector *)block;
			bs->fat_length = FAT2CPU16(bs->fat_length);

			if (bs->fat_length == 0)
				volinfo = (volume_info *)(block + sizeof(boot_sector));
			else
				volinfo = (volume_info *)&(bs->fat32_length);
			int len = strlen(volume);
			if (!strncmp(volinfo->volume_label, volume, len))
				return part_num;
		}

		if ((ext_part_sector == 0) ||
				(pt->sys_ind != 0 && !is_extended (pt->sys_ind))) {
			part_num++;
		}
	}

	pt = (dos_partition_t *) (buffer + DOS_PART_TBL_OFFSET);
	for (i = 0; i < 4; i++, pt++) {
		if (is_extended (pt->sys_ind)) {
			int lba_start = le32_to_int (pt->start4) + relative;
			return get_dos_partitions_index(dev_desc, volume, part_num,
					lba_start, ext_part_sector == 0 ? lba_start : relative);
		}
	}
	return -1;
}

int get_partition_by_volume(const char *volume, block_dev_desc_t *dev_desc)
{
	return get_dos_partitions_index(dev_desc, volume, 1, 0, 0);
}

void print_part_dos (block_dev_desc_t *dev_desc)
{
	printf("Part\tStart Sector\tNum Sectors\tUUID\t\tType\n");
	print_partition_extended(dev_desc, 0, 0, 1, 0);
}

int get_partition_info_dos (block_dev_desc_t *dev_desc, int part, disk_partition_t * info)
{
	return get_partition_info_extended(dev_desc, 0, 0, 1, part, info, 0);
}


#endif
