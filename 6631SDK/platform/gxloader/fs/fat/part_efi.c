#include <fat.h>
#include <usb.h>
#include <stdio.h>
#include <string.h>
#include "part_efi.h"

#define PAD_COUNT(s, pad) (((s) - 1) / (pad) + 1)
#define PAD_SIZE(s, pad) (PAD_COUNT(s, pad) * pad)
#define ALLOC_ALIGN_BUFFER_PAD(type, name, size, align, pad)        \
    char __##name[ROUND(PAD_SIZE((size) * sizeof(type), pad), align)  \
              + (align - 1)];                   \
                                    \
    type *name = (type *)ALIGN((uintptr_t)__##name, align)
#define ALLOC_CACHE_ALIGN_BUFFER_PAD(type, name, size, pad)     \
    ALLOC_ALIGN_BUFFER_PAD(type, name, size, ARCH_DMA_MINALIGN, pad)

#define le64_to_cpu(x) (x)

#define BLOCK_CNT(size, blk_desc) (PAD_COUNT(size, blk_desc->blksz))
#define PAD_TO_BLOCKSIZE(size, blk_desc) \
    (PAD_SIZE(size, blk_desc->blksz))

/**
 * efi_crc32() - EFI version of crc32 function
 * @buf: buffer to calculate crc32 of
 * @len - length of buf
 *
 * Description: Returns EFI-style CRC32 value for @buf
 */
static inline u32 efi_crc32(const void *buf, u32 len)
{
    return crc32(0, buf, len);
}

static int validate_gpt_entries(gpt_header *gpt_h, gpt_entry *gpt_e)
{
    uint32_t calc_crc32;

    /* Check the GUID Partition Table Entry Array CRC */
    calc_crc32 = efi_crc32((const unsigned char *)gpt_e,
            le32_to_cpu(gpt_h->num_partition_entries) *
            le32_to_cpu(gpt_h->sizeof_partition_entry));

    if (calc_crc32 != le32_to_cpu(gpt_h->partition_entry_array_crc32)) {
        printf("%s: 0x%x != 0x%x\n",
               "GUID Partition Table Entry Array CRC is wrong",
               le32_to_cpu(gpt_h->partition_entry_array_crc32),
               calc_crc32);
        return -1;
    }

    return 0;
}

/**
 * alloc_read_gpt_entries(): reads partition entries from disk
 * @dev_desc
 * @gpt - GPT header
 *
 * Description: Returns ptes on success,  NULL on error.
 * Allocates space for PTEs based on information found in @gpt.
 * Notes: remember to free pte when you're done!
 */
static gpt_entry *alloc_read_gpt_entries(block_dev_desc_t *dev_desc,
                     gpt_header *pgpt_head)
{
    size_t count = 0, blk_cnt;
    lbaint_t blk;
    gpt_entry *pte = NULL;

    if (!dev_desc || !pgpt_head) {
        printf("%s: Invalid Argument(s)\n", __func__);
        return NULL;
    }

    count = le32_to_cpu(pgpt_head->num_partition_entries) *
        le32_to_cpu(pgpt_head->sizeof_partition_entry);

    /* Allocate memory for PTE, remember to FREE */
    if (count != 0) {
        pte = memalign(ARCH_DMA_MINALIGN,
                   PAD_TO_BLOCKSIZE(count, dev_desc));
    }

    if (count == 0 || pte == NULL) {
        printf("%s: ERROR: Can't allocate %#lX bytes for GPT Entries\n",
               __func__, (ulong)count);
        return NULL;
    }

    /* Read GPT Entries from device */
    blk = le64_to_cpu(pgpt_head->partition_entry_lba);
    blk_cnt = BLOCK_CNT(count, dev_desc);
    if (dev_desc->block_read(dev_desc->dev, blk, (lbaint_t)blk_cnt, pte) != blk_cnt) {
        printf("*** ERROR: Can't read GPT Entries ***\n");
        free(pte);
        return NULL;
    }
    return pte;
}

static int validate_gpt_header(gpt_header *gpt_h, lbaint_t lba,
        lbaint_t lastlba)
{
    uint32_t crc32_backup = 0;
    uint32_t calc_crc32;

    /* Check the GPT header signature */
    if (le64_to_cpu(gpt_h->signature) != GPT_HEADER_SIGNATURE) {
	debug("%s signature is wrong: 0x%llX != 0x%llX\n",
               "GUID Partition Table Header",
               le64_to_cpu(gpt_h->signature),
               GPT_HEADER_SIGNATURE);
        return -1;
    }

    /* Check the GUID Partition Table CRC */
    memcpy(&crc32_backup, &gpt_h->header_crc32, sizeof(crc32_backup));
    memset(&gpt_h->header_crc32, 0, sizeof(gpt_h->header_crc32));

    calc_crc32 = efi_crc32((const unsigned char *)gpt_h,
        le32_to_cpu(gpt_h->header_size));

    memcpy(&gpt_h->header_crc32, &crc32_backup, sizeof(crc32_backup));

    if (calc_crc32 != le32_to_cpu(crc32_backup)) {
        printf("%s CRC is wrong: 0x%x != 0x%x\n",
               "GUID Partition Table Header",
               le32_to_cpu(crc32_backup), calc_crc32);
        return -1;
    }

    /*
     * Check that the my_lba entry points to the LBA that contains the GPT
     */
    if (le64_to_cpu(gpt_h->my_lba) != lba) {
        printf("GPT: my_lba incorrect: %lld != %lld\n",
               le64_to_cpu(gpt_h->my_lba),
               lba);
        return -1;
    }

    /*
     * Check that the first_usable_lba and that the last_usable_lba are
     * within the disk.
     */
    if (le64_to_cpu(gpt_h->first_usable_lba) > lastlba) {
        printf("GPT: first_usable_lba incorrect: %lld > %lld\n",
               le64_to_cpu(gpt_h->first_usable_lba), lastlba);
        return -1;
    }
    if (le64_to_cpu(gpt_h->last_usable_lba) > lastlba) {
        printf("GPT: last_usable_lba incorrect: %lld > %lld\n",
               le64_to_cpu(gpt_h->last_usable_lba), lastlba);
        return -1;
    }

    return 0;
}

/**
 * is_gpt_valid() - tests one GPT header and PTEs for validity
 *
 * lba is the logical block address of the GPT header to test
 * gpt is a GPT header ptr, filled on return.
 * ptes is a PTEs ptr, filled on return.
 *
 * Description: returns 1 if valid,  0 on error.
 * If valid, returns pointers to PTEs.
 */
static int is_gpt_valid(block_dev_desc_t *dev_desc, u64 lba,
            gpt_header *pgpt_head, gpt_entry **pgpt_pte)
{
    if (!dev_desc || !pgpt_head) {
        printf("%s: Invalid Argument(s)\n", __func__);
        return 0;
    }

    /* Read GPT Header from device */
    if (dev_desc->block_read(dev_desc->dev, (lbaint_t)lba, 1, pgpt_head) != 1) {
        printf("*** ERROR: Can't read GPT header ***\n");
        return 0;
    }

    if (validate_gpt_header(pgpt_head, (lbaint_t)lba, dev_desc->lba))
        return 0;

    /* Read and allocate Partition Table Entries */
    *pgpt_pte = alloc_read_gpt_entries(dev_desc, pgpt_head);
    if (*pgpt_pte == NULL) {
        printf("GPT: Failed to allocate memory for PTE\n");
        return 0;
    }

    if (validate_gpt_entries(pgpt_head, *pgpt_pte)) {
        free(*pgpt_pte);
        return 0;
    }

    /* We're done, all's well */
    return 1;
}

/**
 * is_pte_valid(): validates a single Partition Table Entry
 * @gpt_entry - Pointer to a single Partition Table Entry
 *
 * Description: returns 1 if valid,  0 on error.
 */
static int is_pte_valid(gpt_entry * pte)
{
    efi_guid_t unused_guid;

    if (!pte) {
        printf("%s: Invalid Argument(s)\n", __func__);
        return 0;
    }

    /* Only one validation for now:
     * The GUID Partition Type != Unused Entry (ALL-ZERO)
     */
    memset(unused_guid.b, 0, sizeof(unused_guid.b));

    if (memcmp(pte->partition_type_guid.b, unused_guid.b,
        sizeof(unused_guid.b)) == 0) {

        printf("%s: Found an unused PTE GUID at 0x%08X\n", __func__,
              (unsigned int)(uintptr_t)pte);

        return 0;
    } else {
        return 1;
    }
}

int hex_to_bin(char ch)
{
    if ((ch >= '0') && (ch <= '9'))
        return ch - '0';

    ch = tolower(ch);
    if ((ch >= 'a') && (ch <= 'f'))
        return ch - 'a' + 10;

    return -1;
}

static inline void part_pack_uuid(const u8 *uuid_str, u8 *to)
{
    int i;
    for (i = 0; i < 16; ++i) {
        *to++ = (hex_to_bin(*uuid_str) << 4) |
            (hex_to_bin(*(uuid_str + 1)));
        uuid_str += 2;
        switch (i) {
        case 3:
        case 5:
        case 7:
        case 9:
            uuid_str++;
            continue;
        }
    }
}

static int print_partition_efi(block_dev_desc_t *dev_desc,
                     int ext_part_sector, int relative,
                     int part_num, unsigned int disksig)
{
    ALLOC_CACHE_ALIGN_BUFFER(gpt_header, gpt_head, dev_desc->blksz);
    gpt_entry *gpt_pte = NULL;
    int i = 0;
    char uuid[37];
    unsigned char *uuid_bin;

    /* This function validates AND fills in the GPT header and PTE */
    if (is_gpt_valid(dev_desc, GPT_PRIMARY_PARTITION_TABLE_LBA, gpt_head, &gpt_pte) != 1) {
        printf("%s: *** ERROR: Invalid GPT ***\n", __func__);
        if (is_gpt_valid(dev_desc, (dev_desc->lba - 1),
                 gpt_head, &gpt_pte) != 1) {
            printf("%s: *** ERROR: Invalid Backup GPT ***\n", __func__);
            return 0;
        } else {
            printf("%s: *** Using Backup GPT ***\n", __func__);
        }
    }

    printf("Part\tStart LBA\tEnd LBA\t\tName\n");

    for (i = 0; i < le32_to_cpu(gpt_head->num_partition_entries); i++) {
        /* Stop at the first non valid PTE */
        if (!is_pte_valid(&gpt_pte[i]))
            break;

        printf("%3d\t0x%08llx\t0x%08llx\t\"%s\"\n", (i + 1),
            le64_to_cpu(gpt_pte[i].starting_lba),
            le64_to_cpu(gpt_pte[i].ending_lba),
            "efi");
        printf("\tattrs:\t0x%016llx\n", gpt_pte[i].attributes.raw);
        uuid_bin = (unsigned char *)gpt_pte[i].partition_type_guid.b;
        part_pack_uuid(uuid_bin, uuid);
        printf("\ttype:\t%s\n", uuid);

        uuid_bin = (unsigned char *)gpt_pte[i].unique_partition_guid.b;
        part_pack_uuid(uuid_bin, uuid);
        printf("\tguid:\t%s\n", uuid);
    }

    /* Remember to free pte */
    free(gpt_pte);

    return 1;
}

static efi_guid_t system_guid = PARTITION_SYSTEM_GUID;
static inline int is_bootable(gpt_entry *p)
{
    return p->attributes.fields.legacy_bios_bootable ||
        !memcmp(&(p->partition_type_guid), &system_guid,
            sizeof(efi_guid_t));
}

static int get_partition_info_extended_efi(block_dev_desc_t *dev_desc, int ext_part_sector,
                 int relative, int part_num,
                 int which_part, disk_partition_t *info,
                 unsigned int disksig)
{
    ALLOC_CACHE_ALIGN_BUFFER(gpt_header, gpt_head, dev_desc->blksz);
    gpt_entry *gpt_pte = NULL;

    /* "part" argument must be at least 1 */
    if (which_part < 1) {
        printf("%s: Invalid Argument(s)\n", __func__);
        return -1;
    }

    /* This function validates AND fills in the GPT header and PTE */
    if (is_gpt_valid(dev_desc, GPT_PRIMARY_PARTITION_TABLE_LBA,
            gpt_head, &gpt_pte) != 1) {
        printf("%s: *** ERROR: Invalid GPT ***\n", __func__);
        if (is_gpt_valid(dev_desc, (dev_desc->lba - 1), gpt_head, &gpt_pte) != 1) {
            printf("%s: *** ERROR: Invalid Backup GPT ***\n", __func__);
            return -1;
        } else {
            printf("%s: *** Using Backup GPT ***\n", __func__);
        }
    }

    if (which_part > le32_to_cpu(gpt_head->num_partition_entries) ||
        !is_pte_valid(&gpt_pte[which_part - 1])) {
        printf("%s: *** ERROR: Invalid partition number %d ***\n", __func__, which_part);
        free(gpt_pte);
        return -1;
    }

    /* The 'lbaint_t' casting may limit the maximum disk size to 2 TB */
    info->start = (lbaint_t)le64_to_cpu(gpt_pte[which_part - 1].starting_lba);
    /* The ending LBA is inclusive, to calculate size, add 1 to it */
    info->size = (lbaint_t)le64_to_cpu(gpt_pte[which_part - 1].ending_lba) + 1
             - info->start;
    info->blksz = dev_desc->blksz;

    strcpy((char *)info->type, "gxloader");
    info->bootable = is_bootable(&gpt_pte[which_part - 1]);

    /* Remember to free pte */
    free(gpt_pte);
    return 0;
}

int get_partition_info_efi(block_dev_desc_t *dev_desc, int part, disk_partition_t * info)
{
    return get_partition_info_extended_efi(dev_desc, 0, 0, 1, part, info, 0);
}

int print_part_efi(block_dev_desc_t *dev_desc)
{
    printf("\nSearching GPT...\n");
    printf("Part\tStart Sector\tNum Sectors\tUUID\t\tType\n");
    return print_partition_efi(dev_desc, 0, 0, 1, 0);
}
