#include "stdio.h"
#include "string.h"
#include "sys/types.h"
#include "load_kernel.h"

#define CRAMFS_MAGIC   0x28cd3d45

struct cramfs_inode { /* sizeof == 12*/
	u32 mode:16, uid:16;
	u32 size:24, gid:8;
	u32 namelen:6, offset:26;
};

/*
 * Superblock information at the beginning of the FS.
 */
struct cramfs_super {               /* sizeof == 62 == 0x40 + 12 */
	u32 magic;                  /* 0x28cd3d45 - random number */
	u32 size;                   /* Not used.  mkcramfs currently
	                               writes a constant 1<<16 here. */
	u32 flags;                  /* 0 */
	u32 future;                 /* 0 */
	u8 signature[16];           /* "Compressed ROMFS" */
	u8 fsid[16];                /* random number */
	u8 name[16];                /* user-defined name */
	struct cramfs_inode root;   /* Root inode data */
};

extern int cramfs_uncompress_block(void *dst, void *src, int srclen);
extern int cramfs_uncompress_init(void);
extern int cramfs_uncompress_exit(void);

/* Return the header size  */
static int cramfs_get_header_size(void)
{
	return (sizeof(u32));
}

static int cramfs_check_magic(u32 hdr_addr)
{
	u32 magic;

	memcpy(&magic, (void *)hdr_addr, sizeof(u32));

	if (magic == CRAMFS_MAGIC)
		return 0;
	return -1;
}

/* Return the image size  */
static int cramfs_get_image_size(void)
{
	return 0;
}

static long cramfs_uncompress_page(u8 *dest, u8 *src, u32 srclen)
{
	return cramfs_uncompress_block (dest, src, srclen);
}

static inline void update_progress(void)
{
	putc('.');
}

static long cramfs_load_file(u8 *dest, u8 *src, struct cramfs_inode *inode)
{
	long int curr_block;
	u32 *block_ptrs;
	long size, total_size = 0;
	int i;
	char *data = (char *)src;

	update_progress();
	cramfs_uncompress_init ();
	block_ptrs = ((u32 *) data) + inode->offset;
	curr_block = (inode->offset + ((inode->size + 4095) >> 12)) << 2;
	for (i = 0; i < ((inode->size + 4095) >> 12); i++) {
		size = cramfs_uncompress_page(dest, (u8*)(curr_block + data),
				block_ptrs[i] - curr_block);
		if (size < 0) return size;
		dest += size;
		/* Print a '.' every 0x40000 bytes */
		if (((total_size + size) & ~0x3ffff) - (total_size & ~0x3ffff))
			update_progress();
		total_size += size;
		curr_block = block_ptrs[i];
	}
	printf("\n");
	cramfs_uncompress_exit ();

	return total_size;
}

static u32 cramfs_load_kernel(u32 *dest, u32 src, const char *kernel_filename)
{
	struct cramfs_super *super;
	struct cramfs_inode *inode;
	unsigned long next_inode;
	char *name;
	long size;
	char *data;

	printf("cramfs_load_kernel.\n");

	if (kernel_filename == NULL)
		kernel_filename = "vmlinux";
	data = (char *)src;

	size = -1;
	super = (struct cramfs_super *) data;
	next_inode = super->root.offset << 2;

	while (next_inode < (super->root.offset << 2) + super->root.size) {
		inode = (struct cramfs_inode *) (next_inode + data);
		next_inode += (inode->namelen << 2) + sizeof(struct cramfs_inode);

		if (!(inode->mode & 0x8000)) continue; /* Is it a file? */

		/* does it have 5 to 8 characters? */
		if (!inode->namelen == 2) continue;
		name = (char *) (inode + 1);
		if (strlen(kernel_filename) > (inode->namelen << 2))
			continue;
		if (!strncmp(kernel_filename, name, strlen(kernel_filename))) {
			size = cramfs_load_file((u8*)(*dest), (u8 *)src, inode);
			break;
		}
	}

	return size <= 0 ? 0 : size;
}


struct kernel_loader cramfs_load = {
	.get_header_size  = cramfs_get_header_size,
	.check_magic      = cramfs_check_magic,
	.get_image_size   = cramfs_get_image_size,
	.load_kernel      = cramfs_load_kernel,
	.name             = "cramfs"
};

