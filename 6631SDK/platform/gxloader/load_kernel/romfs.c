#include "stdio.h"
#include "string.h"

#include "config.h"
#include "util.h"
#include "zlib.h"
#include "decompress.h"
#include "string.h"
#include "sys/types.h"
#include "load_kernel.h"

/*  extract from linux's romfs.txt :

The layout of the filesystem is the following:

offset      content

    +---+---+---+---+
  0 | - | r | o | m |  \
    +---+---+---+---+   The ASCII representation of those bytes
  4 | 1 | f | s | - |  /    (i.e. "-rom1fs-")
    +---+---+---+---+
  8 |   full size   |   The number of accessible bytes in this fs.
    +---+---+---+---+
 12 |    checksum   |   The checksum of the FIRST 512 BYTES.
    +---+---+---+---+
 16 | volume name   |   The zero terminated name of the volume,
    :               :   padded to 16 byte boundary.
    +---+---+---+---+
 xx |     file  |
    :    headers    :

[...]
 The following bytes are now part of the file system; each file header
 must begin on a 16 byte boundary.

 offset     content

    +---+---+---+---+
  0 | next filehdr|X|   The offset of the next file header
    +---+---+---+---+     (zero if no more files)
  4 |   spec.info   |   Info for directories/hard links/devices
    +---+---+---+---+
  8 |     size      |   The size of this file in bytes
    +---+---+---+---+
 12 |   checksum    |   Covering the meta data, including the file
    +---+---+---+---+     name, and padding
 16 | file name     |   The zero terminated name of the file,
    :               :   padded to 16 byte boundary
    +---+---+---+---+
 xx | file data |
    :       :

Notes:
+ Values are big endian. (ie NOT intel)
+ Checksum : simple sum of the first 512 bytes (or the number of bytes
  accessible, whichever is smaller).  The applied algorithm is the same
  as in the AFFS filesystem, namely a simple sum of the longwords
  (assuming bigendian quantities again).  For details, please consult
  the source.

*/

typedef struct {
	unsigned int next;
	unsigned int info;
	unsigned int size;
	unsigned int crc;
} file_hdr;

// Lookup file in romfs and return file  address (or 0 if no such file)
file_hdr *romfs_lookup(unsigned int *romfs, const char *name, file_hdr * hdrprev)
{
	char *filename;
	char *volume;
	file_hdr *hdr;
	unsigned int next;

	if (hdrprev == NULL) {
		volume = (char *)(romfs + 4);	// +4 32bits words

		while (*volume)
			volume++;
		volume += 1;	/* must account for the terminating null character */
		hdr = (file_hdr *) (((unsigned int)volume + 15) & ~0xF);
	}
	else {
		next = (swab32(hdrprev->next) & ~0xF) >> 2;
		if (next == 0)
			return NULL;
		hdr = (file_hdr *) (romfs + next);
	}

	while (1) {
		filename = ((char *)hdr) + 16;
		next = (swab32(hdr->next) & ~0xF) >> 2;

		// printf("%08x : %s\n", (void *) hdr - (void *) romfs, filename);

		if (name == NULL)
			return hdr;
		else if (strcmp(name, filename) == 0)
			return hdr;
		else if (next == 0)
			return NULL;

		hdr = (file_hdr *) (romfs + next);
	}
}

#define ROMFS_MAGIC_1 ('-' + ('r'<<8) + ('o'<<16) + ('m'<<24))
#define ROMFS_MAGIC_2 ('1' + ('f'<<8) + ('s'<<16) + ('-'<<24))

static unsigned int load_gz(file_hdr * hdr, unsigned int addr)
{
	unsigned char *filename = (unsigned char *)hdr;
	unsigned char *loadaddr = (unsigned char *)addr;
	unsigned int count = swab32(hdr->size);
	unsigned long src_len = count;
	int ret;

	filename += 16;		// point to beginning of file name
	do {
		filename += 16;
	}
	while (*(filename - 1));

	puts("\nDecompress kernel ...");
	ret = decompress(filename, src_len, loadaddr, DECOMPRESS_OUTPUT_MAX_SIZE);

	if (ret == 0)
		puts("ok\r\n");
	else
	{
		puts("fail\r\n");
		return 0;
	}

	return src_len;
}

static int romfs_get_header_size(void)
{
	return (2*sizeof(u32));
}

static int romfs_check_magic(u32 hdr_addr)
{
	u32 magic[2];

	memcpy(magic, (void *)hdr_addr, sizeof(magic));

	if ((magic[0] == ROMFS_MAGIC_1) && (magic[1] == ROMFS_MAGIC_2))
		return 0;

	return -1;
}

/* Return the image size  */
static int romfs_get_image_size(void)
{
	return 0;
}

static u32 romfs_load_kernel(u32 *dest, u32 src, const char *kernel_filename)
{
	file_hdr *hdr;
	printf("romfs_load file %s.\n", kernel_filename);
	if ((hdr = romfs_lookup((unsigned int *)src, kernel_filename, NULL)) && load_gz(hdr, *dest))
		return true;
	else{
		return false;
	}
}

struct kernel_loader romfs_load = {
	.get_header_size  = romfs_get_header_size,
	.check_magic      = romfs_check_magic,
	.get_image_size   = romfs_get_image_size,
	.load_kernel      = romfs_load_kernel,
	.name             = "romfs"
};

