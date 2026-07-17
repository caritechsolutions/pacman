#include "stdio.h"
#include "string.h"
#include "sys/types.h"
#include "load_kernel.h"
#include "decompress.h"
#include "config.h"

/* image head info */
#define IH_MAGIC        0x27051956     /* Image Magic Number     */
#define IH_NMLEN        24             /* Image Name Length      */
#define IH_MAGIC2       (IH_MAGIC + 1) /* Image Magic2 Number    */

/*
 * Legacy format image header,
 * all data in network byte order (aka natural aka bigendian).
 */
struct image_header {
	uint32_t ih_magic;             /* Image Header Magic Number */
	uint32_t ih_hcrc;              /* Image Header CRC Checksum */
	uint32_t ih_time;              /* Image Creation Timestamp  */
	uint32_t ih_size;              /* Image Data Size           */
	uint32_t ih_load;              /* Data  Load  Address       */
	uint32_t ih_ep;                /* Entry Point Address       */
	uint32_t ih_dcrc;              /* Image Data CRC Checksum   */
	uint8_t  ih_os;                /* Operating System          */
	uint8_t  ih_arch;              /* CPU architecture          */
	uint8_t  ih_type;              /* Image Type                */
	uint8_t  ih_comp;              /* Compression Type          */
	uint8_t  ih_name[IH_NMLEN];    /* Image Name                */
	uint32_t ih_magic2;
	uint32_t text_end;
};
static struct image_header uImage_header;

/* Return the header size  */
static int uImage_get_header_size(void)
{
	return (sizeof(struct image_header));
}

static int uImage_check_magic(u32 hdr_addr)
{
	memcpy(&uImage_header, (void *)hdr_addr, sizeof(struct image_header));

	if (htonl(uImage_header.ih_magic) == IH_MAGIC)
		return 0;
	return -1;
}

/* Return the image size  */
static int uImage_get_image_size(void)
{
	return (htonl(uImage_header.ih_size)+sizeof(struct image_header));
}

static u32 uImage_load_kernel(u32 *dest, u32 src, const char *kernel_filename)
{
	int ret = -1;
	unsigned int kernel_offset = 0;

	kernel_offset = sizeof(struct image_header);
	*dest = htonl(uImage_header.ih_load);

	printf("uImage_load_kernel.\n");

	if(uImage_header.ih_comp != 0){
		puts("\nDecompress kernel ...");
		ret = decompress((unsigned char *)(src+kernel_offset),
				htonl(uImage_header.ih_size), (unsigned char *)(*dest),
				DECOMPRESS_OUTPUT_MAX_SIZE);
		if(ret == 0)
			puts("ok\r\n");
		else{
			puts("fail\r\n");
			return 0;
		}
	}
	else {
		memcpy((void *)(*dest), (void *)(src+kernel_offset),
				htonl(uImage_header.ih_size));
	}

	return uImage_header.ih_size;
}

struct kernel_loader uImage_load = {
	.get_header_size  = uImage_get_header_size,
	.check_magic      = uImage_check_magic,
	.get_image_size   = uImage_get_image_size,
	.load_kernel      = uImage_load_kernel,
	.name             = "uImage"
};
