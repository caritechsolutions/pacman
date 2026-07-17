#include "stdio.h"
#include "string.h"
#include "sys/types.h"
#include "load_kernel.h"
#include "config.h"
#include "elf.h"

#define ELF_DEBUG 0

static int elf_get_header_size(void)
{
    return (sizeof(Elf32_Ehdr));
}

static int elf_check_magic(u32 hdr_addr)
{
    Elf32_Ehdr ehdr;

    memcpy(&ehdr, (void*)hdr_addr, sizeof(Elf32_Ehdr));

#if ELF_DEBUG
    printf("ELF Header:\n");
    printf("  ELF type: 0x%x\n", ehdr.e_type);
    printf("  ELF machine: 0x%x\n", ehdr.e_machine);
    printf("  ELF version: 0x%x\n", ehdr.e_version);
    printf("  ELF enter 0x%x\n", ehdr.e_entry);
    printf("  Entry point: 0x%x\n", ehdr.e_entry);
    printf("  Program header offset: %d\n", ehdr.e_phoff);
    printf("  Number of program headers: %d\n", ehdr.e_phnum);
    printf("  Section header offset: %d\n", ehdr.e_shoff);
    printf("  Number of section headers: %d\n", ehdr.e_shnum);
#endif

    // Verify ELF magic number
    if (ehdr.e_ident[EI_MAG0] != ELFMAG0 || ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr.e_ident[EI_MAG2] != ELFMAG2 || ehdr.e_ident[EI_MAG3] != ELFMAG3) {
        printf("Not an ELF file\n");
        return -1;
    }

    // Check ELF class
    if (ehdr.e_ident[EI_CLASS] != ELFCLASS32) {
        printf("Not an ELF32 file\n");
        return -1;
    }

    // Check ELF Type
    if (ehdr.e_type != ET_EXEC) {
        printf("Not an ELF32 EXEC file\n");
        return -1;
    }

#if defined(CONFIG_ARCH_CKMMU)
    if (ehdr.e_machine != EM_RCE) {
        printf("invaled machine %d\n", ehdr.e_machine);
        return -1;
    }
#elif defined(CONFIG_ARCH_ARM)
    if (ehdr.e_machine != EM_ARM) {
        printf("invaled machine %d\n", ehdr.e_machine);
        return -1;
    }
#else
    printf("(%s %d)nonsupport ARCH : %s\n", __func__, __LINE__, CONFIG_ARCH);
    return -1;
#endif

    return 0;
}

static int elf_get_image_size(void)
{
	printf("(%s %d)nonsupport \n", __func__, __LINE__);
	return -1;
}

static u32 elf_load_kernel(u32 *dest, u32 src, const char *kernel_filename)
{
    unsigned int i = 0, size = 0;
    Elf32_Ehdr ehdr = {0};
    Elf32_Phdr phdr = {0};

    printf("elf_load_kernel.\n");
    memcpy(&ehdr, (void*)src, sizeof(Elf32_Ehdr));

    for (i = 0; i < ehdr.e_phnum; i++) {
        memcpy(&phdr, (void*)(src+ehdr.e_phoff+sizeof(phdr)*i), sizeof(phdr));
#if ELF_DEBUG
        printf("Program Header %d:\n", i);
        printf("  Type: %d\n", phdr.p_type);
        printf("  Offset: 0x%x\n", phdr.p_offset);
        printf("  Virtual Address: 0x%x\n", phdr.p_vaddr);
        printf("  Physical Address: 0x%x\n", phdr.p_paddr);
        printf("  File Size: %d\n", phdr.p_filesz);
        printf("  Memory Size: %d\n", phdr.p_memsz);
        printf("  Flags: %d\n", phdr.p_flags);
        printf("  Align: %d\n", phdr.p_align);
#endif
        // Loadable segment
        if (phdr.p_type == PT_LOAD) {
            // Read segment data from file
            memcpy((void *)phdr.p_vaddr, (void*)(src+phdr.p_offset), phdr.p_filesz);
            // Zero out the rest of the memory if necessary
            if (phdr.p_memsz > phdr.p_filesz)
                memset((void *)(phdr.p_vaddr + phdr.p_filesz), 0, phdr.p_memsz - phdr.p_filesz);

            size += phdr.p_memsz;
        }
    }

    *dest = ehdr.e_entry;
    return size;
}

struct kernel_loader elf_load = {
	.get_header_size  = elf_get_header_size,
	.check_magic      = elf_check_magic,
	.get_image_size   = elf_get_image_size,
	.load_kernel      = elf_load_kernel,
	.name             = "elf"
};

