#ifndef __MINIFS_COMPR_H__
#define __MINIFS_COMPR_H__

#include "minifs_def.h"

#define MINIFS_COMPR_ZLIB 1
#define MINIFS_COMPR_LZO  2

#define STREAM_END_SPACE 12

#define MINIFS_COMPR_NONE  0x00

#ifndef PRESET_DICT
#define PRESET_DICT 0x20
#endif

struct minifs_compressor {
	struct list_head next;
	int priority;			/* used by prirority comr. mode */
	char *name;
	char compr;			/* MINIFS_COMPR_XXX */
	int (*init) (void);
	int (*free) (void);
	int (*compress)  (uint8_t *dest, uint32_t *destLen, const uint8_t *source, uint32_t sourceLen);
	int (*uncompress)(uint8_t *dest, uint32_t *destLen, const uint8_t *source, uint32_t sourceLen);
};

int minifs_compress(uint8_t *dest, uint32_t *destLen, const uint8_t *source, uint32_t sourceLen);
int minifs_uncompress(uint8_t *dest, uint32_t *destLen, const uint8_t *source, uint32_t sourceLen);
int minifs_decompress_init(void);
int minifs_decompress_free(void);
uint32_t minifs_crc32(uint32_t crc, const uint8_t *buf, size_t len);

#endif /* __MINIFS_COMPR_H__ */
