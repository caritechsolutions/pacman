#include "stdio.h"
#include "string.h"
#include "sys/types.h"
#include "load_kernel.h"

#ifdef CONFIG_ECOS_OTA
#include "common/gxmalloc.h"
#define page_malloc(size)      GxCore_Malloc(size)
#define page_free(size)        GxCore_Free(size)
#define partition_read(partition, offset, buf, size)   GxCore_PartitionRead(partition, buf, size, offset)
#endif

const struct kernel_loader *loader[] = {
#ifdef UIMAGE_LOAD
	&uImage_load,
#endif
#ifdef ROMFS_LOAD
	&romfs_load,
#endif
#ifdef CRAMFS_LOAD
	&cramfs_load,
#endif
#ifdef ELF_LOAD
	&elf_load,
#endif
	NULL
};
#if 0
int loader_file(struct part_info *part, const char *filename, unsigned int kernel_offset)
{
	int i = 0;
	void *p;
	int hdr_size;
	int img_size;

	for (i = 0; loader[i]; i++) {
		hdr_size = loader[i]->get_header_size();
		if (hdr_size) {
			p = page_malloc(hdr_size);
			partition_read(part->partition, kernel_offset, p, hdr_size);
			if (loader[i]->check_magic((u32)p)) {
				page_free(p);
				continue;
			}
			page_free(p);
			p = NULL;

			img_size = loader[i]->get_image_size();
			if (!img_size)
				img_size = part->partition->partition_size;

			p = page_malloc(img_size);
			partition_read(part->partition, kernel_offset, p, img_size);

			if ((loader[i]->load_kernel(&(part->dest), (u32)p, filename)) == 0) {
				printf("error loading file %s!\n", filename);
				page_free(p);
				return -1;
			}
			page_free(p);
			return 0;
		}
	}
	return -1;
}
#endif

int get_image_len(struct partition_info *partition, unsigned int offset)
{
	int i = 0;
	void *p;
	int hdr_size;
	int img_size;

	for (i = 0; loader[i]; i++) {
		hdr_size = loader[i]->get_header_size();
		if (hdr_size) {
			p = page_malloc(hdr_size);
			partition_read(partition, offset, p, hdr_size);
			if (loader[i]->check_magic((u32)p)) {
				page_free(p);
				continue;
			}
			page_free(p);
			p = NULL;

			img_size = loader[i]->get_image_size();

			return img_size;
		}
	}

	return -1;
}

static void *image = NULL;
int set_image_buff(void *image_buff)
{
	image = image_buff;
}
void* get_image_buff(void)
{
	if(image == NULL)
		return NULL;
	return image;
}

int load_from_memory(struct part_info *part, const char *filename, void *image_buff, unsigned int kernel_offset)
{
	int i = 0;
	int hdr_size;
	if(image_buff == NULL){
		printf("image buff is NULL \n");
		return -1;
	}

	for (i = 0; loader[i]; i++) {
		hdr_size = loader[i]->get_header_size();
		if (hdr_size) {
			if (loader[i]->check_magic((u32)((char *)image_buff+kernel_offset)))
				continue;

			if ((loader[i]->load_kernel(&(part->dest), (u32)((char *)image_buff+kernel_offset), filename)) == 0) {
				printf("load %s failed!\n", loader[i]->name);
				return -1;
			}
			return 0;
		}
	}

	return -1;
}
